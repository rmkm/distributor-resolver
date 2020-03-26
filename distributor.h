#include <string>
#include <ctime>

using namespace std;

class Distributor
{
    public:
        string host, record;
        time_t last_learned;
        Distributor ();
        Distributor (string, string);
        void set_values (string, string);
        void get_current_time();
};

Distributor::Distributor() {
    host = "default_host";
    record = "default_record";
    last_learned = time(nullptr);
}

Distributor::Distributor (string h, string r) {
    host = h;
    record = r;
    last_learned = time(nullptr);
}

void Distributor::set_values (string h, string r) {
    host = h;
    record = r;
}

void Distributor::get_current_time() {
    last_learned = time(nullptr);
}
