#include <iostream>
#include <string>
#include <sys/socket.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include "distributor.h"

using namespace boost::property_tree;
using namespace std;

Distributor::Distributor() {
    topic = "default_topic";
    host = "default_host";
    record = "default_record";
    last_learned = time(nullptr);
}

Distributor::Distributor (string t, string h, string r) {
    topic = t;
    host = h;
    record = r;
    last_learned = time(nullptr);
}

void Distributor::set_values (string t, string h, string r) {
    topic = t;
    host = h;
    record = r;
}

void Distributor::get_current_time() {
    last_learned = time(nullptr);
}

bool is_valid_topic(string topic) {
    return (topic.front() != '/' && topic.back() != '/');
}

ptree create_hello(string topic, string host, string record) {
    ptree pt;
    if (!is_valid_topic(topic)) {
        cout << "ERR: Topic is invalid: " << topic  << endl;
        return pt;
    }
    pt.put("Type", "Hello");
    pt.put("Topic", topic);
    pt.put("Host", host);
    pt.put("Record", record);
    return pt;
}

ptree create_query(string topic) {
    ptree pt;
    if (!is_valid_topic(topic)) {
        cout << "ERR: Topic is invalid" << endl;
        return pt;
    }
    pt.put("Type", "Query");
    pt.put("Topic", topic);
    return pt;
}

ptree create_reply(string topic, string host, string record) {
    ptree pt;
    if (!is_valid_topic(topic)) {
        cout << "ERR: Topic is invalid" << endl;
        return pt;
    }
    pt.put("Type", "Reply");
    pt.put("Topic", topic);
    pt.put("Host", host);
    pt.put("Record", record);
    return pt;
}

bool send_hello(int socket, string topic, string host, string record) {
    ptree pt = create_hello(topic, host, record);
    if (pt.empty()) {
        cout << "ERR: Failed to create hello" << endl;
        return false;
    }
    stringstream ss;
    write_json(ss, pt);
    cout << "==> Send Hello" << endl;
    cout << ss.str() << endl;
    send(socket, ss.str().c_str(), ss.str().length(), 0);
    return true;
}

bool send_query(int socket, string topic) {
    ptree pt = create_query(topic);
    if (pt.empty()) {
        cout << "ERR: Failed to create query" << endl;
        return false;
    }
    stringstream ss;
    write_json(ss, pt);
    cout << "==> Send Query" << endl;
    cout << ss.str() << endl;
    send(socket, ss.str().c_str(), ss.str().length(), 0);
    return true;
}

bool send_reply(int socket, string topic, string host, string record) {
    ptree pt = create_reply(topic, host, record);
    if (pt.empty()) {
        cout << "ERR: Failed to create reply" << endl;
        return false;
    }
    stringstream ss;
    write_json(ss, pt);
    cout << "==> Send Reply" << endl;
    cout << ss.str() << endl;
    send(socket, ss.str().c_str(), ss.str().length(), 0);
    return true;
}
