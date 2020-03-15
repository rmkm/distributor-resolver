//Example code: A simple server side code, which echos back the received message. 
//Handle multiple socket connections with select and fd_set on Linux 
#include <stdio.h> 
#include <string.h> //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h> //close 
#include <arpa/inet.h> //close 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#define TRUE 1 
#define FALSE 0 
#define PORT 8080 

#include <iostream>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/unordered_map.hpp>
#include <utility>
#include "distributor.h"
using namespace boost::property_tree;
using namespace std;
typedef boost::unordered_map<string, Distributor> distributor_map;

bool is_valid_topic(string topic) {
    return (topic.front() != '/' && topic.back() != '/');
}

ptree create_hello(string topic, string host, string record) {
    ptree pt;
    if (!is_valid_topic(topic)) {
        cout << "ERR: Topic is invalid" << endl;
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

//bool insert_dist(distributor_map map, string topic, string host, string record) {
//    if (!is_valid_topic(topic)) {
//        cout << "ERR: Topic is invalid" << endl;
//        return false;
//    }
//    Distributor distributor;
//    pair <string, Distributor> pair;
//    distributor.host= host;
//    distributor.record = record;
//    pair = make_pair(topic, distributor);
//    map.insert(pair);
//    return true;
//}
    
int main(int argc , char *argv[]) 
{ 
    int opt = TRUE; 
    int master_socket, addrlen, new_socket, client_socket[30],
        max_clients = 30 , activity, i , valread , sd; 
    int max_sd; 
    int my_topic_level;
    struct sockaddr_in address; 
    Distributor distributor;
    pair <string, Distributor> pair;
    string query_topic = "regionA/blockB/buildingC";
        
    char buffer[1025]; //data buffer of 1K 
        
    //set of socket descriptors 
    fd_set readfds; 
        
    //a message 
    char *message = "ECHO Daemon v1.0 \r\n"; 

    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++) 
    { 
        client_socket[i] = 0; 
    } 

    typedef boost::unordered_map<string, Distributor> distributor_map;
    distributor_map dist_map;
    ptree pt;
    read_json("conf.json", pt);

    // Parse JSON file
    boost::optional<string> my_topic = pt.get_optional<string>("Me.Topic");
    boost::optional<string> my_record= pt.get_optional<string>("Me.Record");
    boost::optional<string> my_host= pt.get_optional<string>("Me.Host");
    boost::optional<string> root = pt.get_optional<string>("root");
    boost::optional<string> parent = pt.get_optional<string>("parent");
    boost::optional<string> test_code = pt.get_optional<string>("test_code");
    if (!my_topic) {
        cerr << "Topic is not specified" << endl;
        exit(EXIT_FAILURE);
    }
    if (!my_record) {
        cerr << "Record is not specified" << endl;
        exit(EXIT_FAILURE);
    }
    if (!my_host) {
        cerr << "Host is not specified" << endl;
        exit(EXIT_FAILURE);
    }
    if ((*my_topic).at(0) == '/') {
        cerr << "ERR: First character of topic is '/'" << endl;
        exit(EXIT_FAILURE);
    }
    // Insert my info to dist map
    //insert_dist(dist_map, *my_topic, *my_host, *my_record);
    distributor.set_values(*my_host, *my_record);
    //distributor.host= *my_host;
    //distributor.record = *my_record;
    pair = make_pair(*my_topic, distributor);
    dist_map.insert(pair);
    my_topic_level = count((*my_topic).begin(), (*my_topic).end(), '/');
    cout << "My topic level is " << my_topic_level << endl;

    BOOST_FOREACH (const ptree::value_type& child, pt.get_child("distributor_list")) {
        const ptree& entry = child.second;
        boost::optional<string> topic = entry.get_optional<string>("Topic");
        boost::optional<string> host = entry.get_optional<string>("Host");
        boost::optional<string> record = entry.get_optional<string>("Record");

        if (!topic) {
            cerr << "Topic is nothing" << endl;
            exit(EXIT_FAILURE);
        }
        if (!host) {
            cerr << "Host is nothing" << endl;
            exit(EXIT_FAILURE);
        }
        if (!record) {
            cerr << "Record is nothing" << endl;
            exit(EXIT_FAILURE);
        }

        Distributor db (*host, *record);;
        //pair <string, Distributor> pair = make_pair(*topic, db);
        pair = make_pair(*topic, db);
        dist_map.insert(pair);
    }

    cout << "==> dist_map" << endl;
    for (auto x : dist_map) {
        cout << "    " << x.first << " " << x.second.host << " " << x.second.record << endl;
    }
    cout << endl;
    
    // Connect to parent to say hello
    if (parent) {
        int sock = 0;
        struct sockaddr_in serv_addr;
        char buffer[1024] = {0};
        ptree pt_hello, pt_test;
        stringstream ss;
        //Distributor distributor;

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return -1;
        }
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        if(inet_pton(AF_INET, (*parent).c_str(), &serv_addr.sin_addr)<=0) {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("\nConnection Failed \n");
            return -1;
        }
        // Create hello JSON;
        send_hello(sock, *my_topic, *my_host, *my_record);
        //pt_hello.put("Type", "Hello");
        //pt_hello.put("Topic", *my_topic);
        //pt_hello.put("Record", *my_record);
        //pt_hello.put("Host", *my_host);
        //write_json(ss, pt_hello);
        //cout << "Hello message:" << endl << ss.str().c_str() << endl;
        //send(sock, ss.str().c_str(), ss.str().length(), 0);
        //cout << "Hello message sent" << endl;

        read(sock , buffer, 1024);
        cout << "==> Receved data" << endl << buffer << endl;
        // Parse JSON
        //ptree pt;
        //stringstream ss_recv;
        //stringstream ss_send;
        //ss_recv << buffer;
        ss.str("");
        ss << buffer;
        read_json(ss, pt_hello);
        boost::optional<string> type = pt_hello.get_optional<string>("Type");
        boost::optional<string> topic = pt_hello.get_optional<string>("Topic");
        boost::optional<string> record = pt_hello.get_optional<string>("Record");
        boost::optional<string> host= pt_hello.get_optional<string>("Host");
        if (type) {
            if (*type == "Hello") {
                distributor.set_values(*host, *record);
                //distributor.host = *host;
                //distributor.record = *record;
                //pair <string, Distributor> pair = make_pair(*topic, distributor);
                pair = make_pair(*topic, distributor);
                dist_map.insert(pair);
                cout << "ADD Topic: " << *topic << ", Host: " << *host << ", Record: " << *record << endl;
            } else {
                cout << "Received wrong hello message" << endl;
            }
        }

        //add new socket to array of sockets 
        for (i = 0; i < max_clients; i++) { 
            //if position is empty 
            if( client_socket[i] == 0 ) 
            { 
                client_socket[i] = sock; 
                printf("Adding to list of sockets as %d\n" , i); 
                break; 
            } 
        }
        //clear the socket set 
        FD_ZERO(&readfds); 
        FD_SET(sock , &readfds); 

        //Test code start
        if (test_code && *test_code == "yes") {
            // create Query
            send_query(sock, query_topic);
            //ss.str("");
            //pt_test = create_query("regionA/blockA/buildingB");
            ////pt_test.put("Type", "Query");
            ////string test_topic = "regionA/blockA";
            ////pt_test.put("Topic", test_topic);
            //if (!pt_test.empty()) {
            //    write_json(ss, pt_test);
            //} else {
            //    cerr << "ERR: Could not create query" << endl;
            //    exit(EXIT_FAILURE); 
            //}
            //cout << "Test message:" << endl << ss.str().c_str() << endl;
            //send(sock, ss.str().c_str(), ss.str().length(), 0);
        }
        //Test code end
    }



    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
        sizeof(opt)) < 0 ) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    
    //type of socket created 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
        
    //bind the socket to localhost port 8888 
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    printf("Listener on port %d \n", PORT); 
        
    //try to specify maximum of 3 pending connections for the master socket 
    if (listen(master_socket, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
        
    //accept the incoming connection 
    addrlen = sizeof(address); 
    puts("Waiting for connections ..."); 
        
    while(TRUE) 
    { 
        //clear the socket set 
        FD_ZERO(&readfds);
    
        //add master socket to set 
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
            
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++) 
        { 
            //socket descriptor 
            sd = client_socket[i]; 
                
            //if valid socket descriptor then add to read list 
            if (sd > 0) {
                FD_SET( sd , &readfds); 
            }
                
            //highest file descriptor number, need it for the select function 
            if (sd > max_sd) {
                max_sd = sd; 
            }
        } 
    
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely 
        activity = select(max_sd + 1 , &readfds , NULL , NULL , NULL); 
    
        if ((activity < 0) && (errno!=EINTR)) 
        { 
            printf("select error"); 
        } 
            
        //If something happened on the master socket , 
        //then its an incoming connection 
        if (FD_ISSET(master_socket, &readfds)) 
        { 
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
            { 
                perror("accept"); 
                exit(EXIT_FAILURE); 
            } 
            
            //inform user of socket number - used in send and receive commands 
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port)); 
        
            //send new connection greeting message 
            //if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
            //{ 
            //    perror("send"); 
            //} 
            //    
            //puts("Welcome message sent successfully"); 
                
            //add new socket to array of sockets 
            for (i = 0; i < max_clients; i++) 
            { 
                //if position is empty 
                if( client_socket[i] == 0 ) 
                { 
                    client_socket[i] = new_socket; 
                    printf("Adding to list of sockets as %d\n" , i); 
                        
                    break; 
                } 
            } 
        } 
            
        //else its some IO operation on some other socket 
        for (i = 0; i < max_clients; i++) 
        { 
            sd = client_socket[i]; 
                
            if (FD_ISSET( sd , &readfds)) 
            { 
                //Check if it was for closing , and also read the 
                //incoming message 
                if ((valread = read( sd , buffer, 1024)) == 0) 
                { 
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen); 
                    printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr) , ntohs(address.sin_port)); 
                        
                    //Close the socket and mark as 0 in list for reuse 
                    close( sd ); 
                    client_socket[i] = 0; 
                } 
                    
                //Echo back the message that came in 
                else
                { 
                    buffer[valread] = '\0'; 
                    cout << "==> Received data" << endl << buffer << endl;
                    // Parse JSON
                    ptree pt;
                    stringstream ss_recv;
                    stringstream ss_send;
                    //Distributor distributor;
                    ss_recv << buffer;
                    read_json(ss_recv, pt);
                    boost::optional<string> type = pt.get_optional<string>("Type");
                    boost::optional<string> topic = pt.get_optional<string>("Topic");
                    boost::optional<string> record = pt.get_optional<string>("Record");
                    boost::optional<string> host= pt.get_optional<string>("Host");
                    if (!type || !topic) {
                        cout << "ERR: Wrong JSON format, cannot parse" << endl;
                        continue;
                    }
                    if ((*topic).at(0) == '/') {
                        cout << "ERR: First character of topic is '/'" << endl;
                        continue;
                    }
                    if (*type == "Hello") {
                        cout << "Received Hello message" << endl;
                        if (!is_valid_topic(*topic)) {
                            cerr << "ERR: Topic begin or finish with '/'" << endl;
                            exit(EXIT_FAILURE);
                        }
                        ptree pt_hello;
                        distributor.set_values(*host, *record);
                        //distributor.host = *host;
                        //distributor.record = *record;
                        //pair <string, Distributor> pair = make_pair(*topic, distributor);
                        pair = make_pair(*topic, distributor);
                        dist_map.insert(pair);
                        cout << "ADD Topic: " << *topic << ", Host: " << *host << ", Record: " << *record << endl;
                        // Create hello
                        pt_hello.put("Type", "Hello");
                        pt_hello.put("Topic", *my_topic);
                        pt_hello.put("Host", *my_host);
                        pt_hello.put("Record", *my_record);
                        write_json(ss_send, pt_hello);
                        send(sd, ss_send.str().c_str(), ss_send.str().length(), 0);
                        cout << "Hello message sent" << endl;
                    } else if (*type == "Query") {
                        int topic_level = count((*topic).begin(), (*topic).end(), '/'); // num of '/'
                        if (topic_level > 0) {
                            size_t found = (*topic).length();
                            for (int i = 0; i < topic_level+1; i++) {
                                cout << "Try to find " << (*topic).substr(0, found) << endl;
                                try {
                                    distributor = dist_map.at((*topic).substr(0, found));
                                    cout << "==> Found distributor" << endl;
                                    cout << "Topic: " << (*topic).substr(0, found) << endl;
                                    cout << "Host: " << distributor.host << endl;
                                    cout << "Record: " << distributor.record << endl;
                                    cout << endl;
                                    break;
                                }
                                catch (const out_of_range &e) {
                                    found = (*topic).find_last_of('/');      
                                    continue;
                                }
                            }
                            //create reply
                            send_reply(sd, (*topic).substr(0, found), distributor.host, distributor.record);
                            //ptree pt_reply;
                            //pt_reply.put("Type", "Reply");
                            //pt_reply.put("Topic", (*topic).substr(0, found));
                            //pt_reply.put("Host", distributor.host);
                            //pt_reply.put("Record", distributor.record);
                            //write_json(ss_send, pt_reply);
                            //cout << "==> Send Reply" << endl;
                            //cout << ss_send.str() << endl;
                            //send(sd, ss_send.str().c_str(), ss_send.str().length(), 0);
                        } else {
                            cerr << "ERR: Wrong topic format: " << *topic;
                            exit(EXIT_FAILURE);
                        }
                    } else if (*type == "Reply" ) {
                        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen); 
                        string ip_addr = inet_ntoa(address.sin_addr);
                        // insert pair
                        distributor.set_values(*host, *record);
                        pair = make_pair(*topic, distributor);
                        dist_map.insert(pair);
                        if (ip_addr == *host) {
                            cout << "==> Resolve finish" << endl;
                        } else {
                            cout << "==> Continue resolving" << endl;
                            //create socket
                            int sock = 0;
                            struct sockaddr_in serv_addr;
                            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                                printf("\n Socket creation error \n");
                                return -1;
                            }
                            serv_addr.sin_family = AF_INET;
                            serv_addr.sin_port = htons(PORT);
                            //if(inet_pton(AF_INET, ip_addr.c_str(), &serv_addr.sin_addr)<=0) {
                            if(inet_pton(AF_INET, (*host).c_str(), &serv_addr.sin_addr)<=0) {
                                printf("\nInvalid address/ Address not supported \n");
                                return -1;
                            }
                            if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                                printf("\nConnection Failed \n");
                                return -1;
                            }

                            //add new socket to array of sockets 
                            for (i = 0; i < max_clients; i++) { 
                                //if position is empty 
                                if( client_socket[i] == 0 ) 
                                { 
                                    client_socket[i] = sock; 
                                    printf("Adding to list of sockets as %d\n" , i); 
                                    break; 
                                } 
                            }
                            FD_SET(sock , &readfds); 
                            send_query(sock, query_topic);
                        }
                    }

                    //set the string terminating NULL byte on the end 
                    //of the data read 
                    //buffer[valread] = '\0'; 
                    //send(sd , buffer , strlen(buffer) , 0 ); 
                } 
            } 
        } 
    } 
        
    return 0; 
} 

