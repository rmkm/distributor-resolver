#ifndef DISTRIBUTOR_H
#define DISTRIBUTOR_H

#include <string>
#include <ctime>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost::property_tree;
using namespace std;

class Distributor
{
    public:
        string topic, host, record;
        time_t last_learned;
        Distributor ();
        Distributor (string, string, string);
        void set_values (string, string, string);
        void get_current_time();
};

//Distributor::Distributor() {
//    topic = "default_topic";
//    host = "default_host";
//    record = "default_record";
//    last_learned = time(nullptr);
//}
//
//Distributor::Distributor (string t, string h, string r) {
//    topic = t;
//    host = h;
//    record = r;
//    last_learned = time(nullptr);
//}
//
//void Distributor::set_values (string t, string h, string r) {
//    topic = t;
//    host = h;
//    record = r;
//}
//
//void Distributor::get_current_time() {
//    last_learned = time(nullptr);
//}

//Distributor::Distributor();
//Distributor::Distributor (string t, string h, string r);
//void Distributor::set_values (string t, string h, string r);
//void Distributor::get_current_time();
bool is_valid_topic(string topic);
ptree create_hello(string topic, string host, string record);
ptree create_query(string topic);
ptree create_reply(string topic, string host, string record);
bool send_hello(int socket, string topic, string host, string record);
bool send_query(int socket, string topic);
bool send_reply(int socket, string topic, string host, string record);

#endif
