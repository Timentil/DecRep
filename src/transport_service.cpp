#include "transport_service.hpp"
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/uuid.hpp>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace zlib = boost::iostreams::zlib;
using tcp = net::ip::tcp;

http::response<http::string_body> transport_service::Server::handle_response(
    const http::request<http::string_body> &req,
    const std::string &client_address
) const {
    http::response<http::string_body> res{http::status::ok, req.version()};

    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    logger.log(
        std::string("Received request from ") + client_address + ": " +
        std::string(req.method_string()) + " " + std::string(req.target()) +
        '\n'
    );

    if (req.method() == http::verb::get) {
        const std::string file_path = dec_rep_path + std::string(req.target());
        std::ifstream file(file_path);
        if (!file) {
            res.result(http::status::not_found);
            res.body() = "File not found";
            logger.log(std::string("File not found: ") + file_path + '\n');
        } else {
            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
            );
            file.close();
            const int current_compression_level = compression_level;
            if (current_compression_level > Z_BEST_SPEED - 1) {
                std::string compressed =
                    deflate_compress(content, current_compression_level);
                res.body() = compressed;
                res.set(
                    "X-Compression-Level",
                    std::to_string(current_compression_level)
                );
                res.set(http::field::content_encoding, "deflate");
            } else {
                res.body() = content;
                res.set(http::field::content_encoding, "raw");
            }

            logger.log(
                std::string("Compression level: ") +
                std::to_string(current_compression_level) + '\n'
            );
            std::string hash = sha1_hash_file(file_path);
            logger.log("Hash: " + hash + '\n');

            if (!hash.empty() && hash.find_first_not_of("0123456789abcdefABCDEF"
                                 ) == std::string::npos) {
                res.set(http::field::etag, hash);
            } else {
                logger.log("Invalid ETag value generated: " + hash + '\n');
            }
        }
    } else if (req.method() == http::verb::post) {
        const std::string file_path = dec_rep_path + std::string(req.target());
        get_file(
            client_address, std::string(req.target()), dec_rep_path,
            get_local_time(file_path)
        );
        logger.log("Get file: " + file_path + '\n');
    } else {
        res.result(http::status::bad_request);
        res.body() = "Invalid request";
        logger.log(
            "Invalid request: " + std::string(req.method_string()) + '\n'
        );
    }

    logger.log(std::string("Sending response") + '\n');
    res.prepare_payload();
    return res;
}

void transport_service::Server::do_session(tcp::socket &socket) const {
    try {
        ssl::context &ctx =
            Certificate_Singleton::get_instance().get_server_context();
        const std::string client_address =
            socket.remote_endpoint().address().to_string();
        beast::ssl_stream<beast::tcp_stream> stream(std::move(socket), ctx);

        stream.handshake(ssl::stream_base::server);

        beast::flat_buffer buffer;

        for (;;) {
            http::request<http::string_body> req;
            http::read(stream, buffer, req);

            auto res = handle_response(req, client_address);

            http::write(stream, res);
            if (!res.keep_alive()) {
                break;
            }
        }
        beast::error_code ec;
        stream.shutdown(ec);
        if (ec == net::error::eof) {
            ec = {};
        }
        if (ec == ssl::error::stream_truncated) {
            ec = {};
        }
        if (ec) {
            throw beast::system_error{ec};
        }
    } catch (const beast::system_error &se) {
        if (se.code() != http::error::end_of_stream) {
            logger.log("Session error: " + std::string(se.what()) + '\n');
        }
    } catch (const std::exception &e) {
        logger.log("Session error: " + std::string(e.what()) + '\n');
    }
}

void transport_service::Server::run() const {
    try {
        net::io_context ioc{thread_count};
        std::vector<std::thread> v;
        for (auto i = thread_count - 1; i > 0; --i) {
            v.emplace_back([&ioc] { ioc.run(); });
        }

        tcp::acceptor acceptor{ioc};
        boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::tcp::v4(), port
        );
        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true)
        );
        acceptor.bind(endpoint);
        acceptor.listen();

        logger.log("Server is running on port " + port + '\n');

        for (;;) {
            logger.log("Waiting for a connection..." + '\n');
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            logger.log("Handle response" + '\n');
            std::thread(
                [this](tcp::socket m_socket) { do_session(m_socket); },
                std::move(socket)
            )
                .detach();
        }
    } catch (const std::exception &e) {
        logger.log("Error: " + std::string(e.what()) + '\n');
    }
}

void transport_service::get_file(
    const std::string &server_address,
    const std::string &file_name,
    const std::string &file_path,
    const unsigned long local_clock
) {
    try {
        ssl::context &ctx =
            Certificate_Singleton::get_instance().get_client_context();
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        const auto results =
            resolver.resolve(server_address, std::to_string(SERVER_PORT));

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
        if (!SSL_set_tlsext_host_name(
                stream.native_handle(), server_address.c_str()
            )) {
            beast::error_code ec{
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{
            http::verb::get, "/" + file_name, 11};
        req.set(http::field::host, server_address);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set("X-File-Version", std::to_string(local_clock));
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        if (res.result() == http::status::ok) {
            std::ofstream out_file(file_path + file_name, std::ios::binary);
            if (!out_file) {
                std::cerr << "Failed to open file for writing: "
                          << file_path + file_name << std::endl;
                return;
            }

            std::string file_data = beast::buffers_to_string(res.body().data());

            if (res.base().find(http::field::content_encoding) !=
                    res.base().end() &&
                res.base().at(http::field::content_encoding) == "deflate") {
                file_data = deflate_decompress(file_data);
            }
            out_file << file_data;
            out_file.close();
            // out_file << beast::buffers_to_string(res.body().data());
            out_file.close();
            std::string etag = res.base().at(http::field::etag);
            std::string hash = sha1_hash_file(file_path + file_name);
            if (etag != hash) {
                std::cerr << "File doesn't match expected hash" << std::endl;
                std::cerr << file_path + file_name + " hash: " << hash
                          << std::endl;
                std::cerr << "Expected ETag: " << etag << std::endl;
                throw std::runtime_error("File doesn't match expected hash");
            }

            std::cout << "File downloaded successfully: "
                      << file_path + file_name << std::endl;
        } else {
            std::cerr << "Error: " << res.result() << " - " << res.reason()
                      << std::endl;
        }

        beast::error_code ec;
        stream.shutdown(ec);
        if (ec == net::error::eof) {
            ec = {};
        }
        if (ec == ssl::error::stream_truncated) {
            ec = {};
        }
        if (ec) {
            throw beast::system_error{ec};
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void transport_service::send_file(
    const std::string &server_address,
    const std::string &file_name,
    const std::string &file_path,
    const unsigned long local_clock
) {
    try {
        ssl::context &ctx =
            Certificate_Singleton::get_instance().get_client_context();
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        const auto results =
            resolver.resolve(server_address, std::to_string(SERVER_PORT));

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
        if (!SSL_set_tlsext_host_name(
                stream.native_handle(), server_address.c_str()
            )) {
            beast::error_code ec{
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{
            http::verb::post, file_path + "/" + file_name, 11};
        req.set(http::field::host, server_address);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);
        if (res.result() != http::status::ok) {
            std::cerr << "Error: " << res.result() << " - " << res.reason()
                      << std::endl;
        }

        beast::error_code ec;
        stream.shutdown(ec);
        if (ec == net::error::eof) {
            ec = {};
        }
        if (ec == ssl::error::stream_truncated) {
            ec = {};
        }
        if (ec) {
            throw beast::system_error{ec};
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

[[nodiscard]] std::string transport_service::sha1_hash_file(
    const std::string &filename
) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return {};
    }

    std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(buffer.data(), buffer.size());

    boost::uuids::detail::sha1::digest_type digest;
    sha1.get_digest(digest);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (unsigned int i : digest) {
        unsigned int value = ((i & 0xFF000000) >> 24) |
                             ((i & 0x00FF0000) >> 8) | ((i & 0x0000FF00) << 8) |
                             ((i & 0x000000FF) << 24);

        oss << std::setw(8) << value;
    }
    return oss.str();
}

[[nodiscard]] std::string transport_service::deflate_compress(
    const std::string &data,
    const int compression_level
) {
    z_stream zs{};
    deflateInit(&zs, compression_level);

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    zs.avail_in = data.size();

    int ret;
    char out_buffer[BUFFER_SIZE];
    std::string out_string;

    do {
        zs.next_out = reinterpret_cast<Bytef *>(out_buffer);
        zs.avail_out = sizeof(out_buffer);

        ret = deflate(&zs, Z_FINISH);

        if (out_string.size() < zs.total_out) {
            out_string.append(out_buffer, zs.total_out - out_string.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);
    return out_string;
}

[[nodiscard]] std::string transport_service::deflate_decompress(
    const std::string &data
) {
    z_stream zs{};
    inflateInit(&zs);

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
    zs.avail_in = data.size();

    int ret;
    char out_buffer[32768];
    std::string out_string;

    do {
        zs.next_out = reinterpret_cast<Bytef *>(out_buffer);
        zs.avail_out = sizeof(out_buffer);

        ret = inflate(&zs, 0);

        if (out_string.size() < zs.total_out) {
            out_string.append(out_buffer, zs.total_out - out_string.size());
        }
    } while (ret == Z_OK);

    inflateEnd(&zs);
    return out_string;
}