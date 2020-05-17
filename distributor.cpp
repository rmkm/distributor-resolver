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
    topic = "default";
    host = "default";
    record = "default";
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

void Distributor::set_host(string h) {
    host = h;
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

ptree create_reply(string req_topic, string topic, string host, string record) {
    ptree pt;
    if (!is_valid_topic(topic)) {
        cout << "ERR: Topic is invalid: " << topic << endl;
        return pt;
    }
    if (!is_valid_topic(req_topic)) {
        cout << "ERR: Topic is invalid: " << req_topic << endl;
        return pt;
    }
    pt.put("Type", "Reply");
    pt.put("Requested_topic", req_topic);
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

bool send_reply(int socket, string req_topic, string topic, string host, string record) {
    ptree pt = create_reply(req_topic, topic, host, record);
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

// Return IP address
Distributor search_distributor (distributor_map dist_map, string topic) {
    Distributor distr;
    if (!is_valid_topic(topic)) {
        cout << "ERR: Topic is invalid" << endl;
        return distr;
    }
    //string target_host;
    string sub_topic;
    int topic_level = count((topic).begin(), (topic).end(), '/'); // num of '/'
    size_t found = (topic).length();
    for (int i = 0; i < topic_level+1; i++) {
        sub_topic = (topic).substr(0, found);
        try {
            cout << "Try to find " << sub_topic << endl;
            distr = dist_map.at(sub_topic);
            cout << "==> Found distributor" << endl;
            cout << "Topic: " << sub_topic << endl;
            cout << "Host: " << distr.host << endl;
            cout << "Record: " << distr.record << endl;
            cout << "Learned: " << asctime(localtime(&(distr.last_learned))) << endl;
            time_t now = time(nullptr);
            cout << now - distr.last_learned << " second past since last time learned";
            cout << endl;
            //target_host = distr.host;
            break;
        }
        catch (const out_of_range &e) {
            //cout << "Next" << endl;
            found = sub_topic.find_last_of('/');
            //cout << "topic: " << sub_topic << endl;
            //cout << "found: " << found << endl;
            if (found > sub_topic.length()) { // Non of distributors matched
                cout << "==> Could not find a distributor for: " << topic << endl;
                cout << "Return root distributor" << endl;
                try {
                    distr = dist_map.at("root");
                }
                catch (exception & e) {
                    cout << "root distributor is not specified" << endl;
                }
                //target_host = distr.host;
                //if (root) {
                //    cout << "Send query to root distributor" << endl;
                //    target_host = *root;
                //} else {
                //    cout << "root distributor is not specified, drop the request" << endl;
                //}
            }
            continue;
        }
    }
    return distr;
    //return target_host;
}
