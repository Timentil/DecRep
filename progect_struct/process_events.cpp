#include "events.cpp"

namespace {
enum class Event {
    Add, 
    DeL
    // ...
}
}; 


int main() {
    DecRec current_session;
    Event x = Event::Add;
    swich(x) {
        case Event::Add {
            // some logic 
            DBManager::add(/* some args*/);
            // some logic
        } break;
        case 
    }
}