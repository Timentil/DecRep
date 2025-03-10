#include "db_manager.cpp"
#include "process_events.cpp"
#include "dec_rep_fs.cpp"
// куча инклудов

namespace {
    enum class Event {
        Add, 
        DeL
        // ...
    }
}; 

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
        swich(x) {
            case Event::Add {
                DecRep::add(d);
            } break;
            case Event::Add {
                DecRep::add();
            } break;
            case Event::Add {
                DecRep::add();
            } break;
            // ...
        }
    }

}