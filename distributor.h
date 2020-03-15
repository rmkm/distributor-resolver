#include <string>

using namespace std;

class Distributor
{
    public:
        string host, record;
        Distributor ();
        Distributor (string, string);
        void set_values (string, string);
};

Distributor::Distributor() {
    host = "default_host";
    record = "default_record";
}

Distributor::Distributor (string h, string r) {
    host = h;
    record = r;
}

void Distributor::set_values (string h, string r) {
    host = h;
    record = r;
}
