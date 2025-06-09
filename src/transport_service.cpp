#include "transport_service.hpp"
#include <boost/uuid.hpp>
#include <iostream>

#include <zlib.h>
#include <boost/iostreams/filter/zlib.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace zlib = boost::iostreams::zlib;
using tcp = net::ip::tcp;

http::response<http::string_body> transport_service::Server::handle_response(
    const http::request<http::string_body> &req
) const {
    http::response<http::string_body> res{http::status::ok, req.version()};

    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());

    if (req.method() == http::verb::get) {
        const std::string file_path = dec_rep_path + std::string(req.target());
        std::ifstream file(file_path);
        if (!file) {
            res.result(http::status::not_found);
            res.body() = "File not found";
        } else {
            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
            );

            std::string compressed = deflate_compress(content);
            res.body() = compressed;
            res.set(http::field::content_encoding, "deflate");
            file.close();
            std::string hash = sha1_hash_file(file_path);
            if (!hash.empty() && hash.find_first_not_of("0123456789abcdefABCDEF"
                                 ) == std::string::npos) {
                res.set(http::field::etag, hash);
            } else {
                std::cerr << "Invalid ETag value generated: " << hash
                          << std::endl;
            }
        }
        } else if (req.method() == http::verb::post) {
            const std::string file_path = dec_rep_path +
            std::string(req.target()); std::ofstream file(file_path);
            if (!file) {
                res.result(http::status::bad_request);
                res.body() = "Failed to create file";
            } else {
                file.write(req.body().data(), static_cast<std::streamsize>(req.body().size()));
                res.body() = "File uploaded successfully";
            }
    } else {
        res.result(http::status::bad_request);
        res.body() = "Invalid request";
    }
    res.prepare_payload();
    return res;
}

void transport_service::Server::do_session(
    tcp::socket &socket,
    ssl::context &ctx
) const {
    try {
        beast::ssl_stream<beast::tcp_stream> stream(std::move(socket), ctx);

        stream.handshake(ssl::stream_base::server);

        beast::flat_buffer buffer;

        for (;;) {
            http::request<http::string_body> req;
            http::read(stream, buffer, req);

            auto res = handle_response(req);

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
            std::cerr << "Session error: " << se.what() << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Session error: " << e.what() << std::endl;
    }
}

void transport_service::Server::run() const {
    try {
        ssl::context ctx(ssl::context::sslv23);
        ctx.set_options(
            ssl::context::default_workarounds | ssl::context::no_sslv2 |
            ssl::context::single_dh_use
        );
        ctx.use_certificate_chain_file("server.crt");
        ctx.use_private_key_file("server.key", ssl::context::pem);
        ctx.use_tmp_dh_file("dhparams.pem");

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

        std::cout << "Server is running on port " << port << std::endl;

        for (;;) {
            std::cout << "Waiting for a connection..." << std::endl;
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread(
                [this](tcp::socket m_socket, ssl::context &m_ctx) {
                    do_session(m_socket, m_ctx);
                },
                std::move(socket), std::ref(ctx)
            )
                .detach();

            std::cout << "Handle response" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void transport_service::get_file(
    const std::string &server_address,
    const std::string &file_name,
    const std::string &file_path,
    ssl::context &ctx
) {
    try {
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

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        if (res.result() == http::status::ok) {
            std::ofstream out_file(file_path +  file_name, std::ios::binary);
            if (!out_file) {
                std::cerr << "Failed to open file for writing: "
                          << file_path + file_name << std::endl;
                return;
            }

            std::string file_data = beast::buffers_to_string(res.body().data());
            if (res.base().find(http::field::content_encoding) != res.base().end() &&
                res.base().at(http::field::content_encoding) == "deflate") {
                file_data = deflate_decompress(file_data);
                }
            out_file << file_data;

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

[[nodiscard]] std::string transport_service::sha1_hash_file(const std::string &filename) {
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

[[nodiscard]] std::string transport_service::deflate_compress(const std::string& data) {
    z_stream zs{};
    deflateInit(&zs, COMPRESSION_LEVEL);

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    zs.avail_in = data.size();

    int ret;
    char out_buffer[BUFFER_SIZE];
    std::string out_string;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(out_buffer);
        zs.avail_out = sizeof(out_buffer);

        ret = deflate(&zs, Z_FINISH);

        if (out_string.size() < zs.total_out) {
            out_string.append(out_buffer, zs.total_out - out_string.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);
    return out_string;
}

[[nodiscard]] std::string transport_service::deflate_decompress(const std::string& data) {
    z_stream zs{};
    inflateInit(&zs);

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    zs.avail_in = data.size();

    int ret;
    char out_buffer[32768];
    std::string out_string;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(out_buffer);
        zs.avail_out = sizeof(out_buffer);

        ret = inflate(&zs, 0);

        if (out_string.size() < zs.total_out) {
            out_string.append(out_buffer, zs.total_out - out_string.size());
        }
    } while (ret == Z_OK);

    inflateEnd(&zs);
    return out_string;
}