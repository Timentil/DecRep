#include <iostream>
#include <pqxx/pqxx>


int main() {
    try {
        //Подключение к бд
        pqxx::connection C("dbname=base user=postgres password=1234 host=localhost");
        if(C.is_open()) {
            std::cout << "connected successfully to " << C.dbname() << std::endl;
        }
        else {
            std::cout << "not really successfully";
            return 1;
        }

        pqxx::work w(C);


        const std::string file = "SELECT f.file_name, fo.file_path, u.username"
                           "FROM FileOwners fo"
                           "JOIN Files f ON fo.file_id = f.id"
                           "JOIN Users u ON fo.owner_id = u.id"
                           "WHERE f.file_name = 'name.txt';";

        pqxx::result res = w.exec(file);


        w.commit();
        C.disconnect();

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}

