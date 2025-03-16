#include "db_manager.cpp"
#include "process_events.cpp"
#include "dec_rep_fs.cpp"
#include <unordered_map>
// куча инклудов

enum class Event {
    ADD,
    DEL,
    DOWNLOAD,
    EXIT,
    INVALID
    // ???
};

const std::unordered_map<std::string, Event> events = {
    {"add", Event::ADD},
    {"add_file", Event::ADD},
    {"add_folder", Event::ADD},
    {"del", Event::DEL},
    {"delete", Event::DEL},
    {"download", Event::DOWNLOAD},
    {"exit", Event::EXIT}};


Event getEvent(const std::string &event) {
    if (const auto it = events.find(event); it != events.end()) {
        return it->second;
    }
    return Event::INVALID;
}

class DecRec {
    DecRep() : {/* init other modules */}
    // file_watcher, HTTP server - "run-time" classes
    // dec_rep_fs init - construct form database
};


int main() {
    DecRep d;
    d.fs

    while () {
        std::getline();

    }

    while () {
        // HTTP server -> Event x;
        switch(event_id) {
            case Event::ADD:
                DecRep::add();
            break;
            case Event::DEL:
                DecRep::del();
            break;
            case Event::DOWNLOAD:
                DecRep::download();
            case Event::EXIT:
                decRep::exit();
            case Event::INVALID:
                DecRep::invalid();
        }
    }

}