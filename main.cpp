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
#include <ctime>
#include <thread>
#include <chrono>
#include "distributor.h"
using namespace boost::property_tree;
using namespace std;
typedef boost::unordered_map<string, Distributor> distributor_map;

int main(int argc , char *argv[]) 
{ 
    int opt = TRUE; 
    int master_socket, addrlen, new_socket, client_socket[30],
        max_clients = 30 , activity, i , valread , sd; 
    int max_sd; 
    int my_topic_level;
    struct sockaddr_in address; 
    //Distributor distributor;
    Distributor distr;
    pair<string, int> ts_pair; // topic : socket
    pair<string, Distributor> pair;
    string query_topic = "regionA/blockB/buildingC";
        
    char buffer[1025]; //data buffer of 1K 
        
    //set of socket descriptors 
    fd_set readfds; 
        
    //a message 
    //char *message = "ECHO Daemon v1.0 \r\n"; 

    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++) 
    { 
        client_socket[i] = 0; 
    } 

    typedef boost::unordered_map<string, int> client_map;
    client_map cli_map;

    //typedef boost::unordered_map<string, Distributor> distributor_map;
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
    distr.set_values(*my_topic, *my_host, *my_record);
    distr.get_current_time();
    pair = make_pair(*my_topic, distr);
    dist_map.insert(pair);
    my_topic_level = count((*my_topic).begin(), (*my_topic).end(), '/');
    cout << "My topic level is " << my_topic_level << endl;

    if (root) {
        Distributor rootd;
        rootd.set_host(*root);
        dist_map.insert(make_pair("root", rootd)); // key is "root"
    }

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

        //Distributor db (*host, *record);;
        //pair <string, Distributor> pair = make_pair(*topic, db);
        //pair = make_pair(*topic, db);
        //dist_map.insert(pair);
        distr.set_values(*topic, *host, *record);
        distr.get_current_time();
        pair = make_pair(*topic, distr);
        dist_map.insert(pair);
    }

    cout << "==> map" << endl;
    for (auto x : dist_map) {
        cout << "    " << x.first << " " << x.second.host << " " << x.second.record
             << "    " << asctime(localtime(&(x.second.last_learned)));
    }
    
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
                distr.set_values(*topic, *host, *record);
                pair = make_pair(distr.topic, distr);
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
        if (test_code && *test_code == "true") {
            // create Query
            send_query(sock, query_topic);
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
                    
                //Process message from other distributors 
                else { 
                    buffer[valread] = '\0'; 
                    cout << endl << "==> Received data" << endl << buffer << endl;
                    // Parse JSON
                    ptree pt;
                    stringstream ss_recv;
                    stringstream ss_send;
                    //Distributor distributor;
                    ss_recv << buffer;
                    string str_data = ss_recv.str();

                    if (str_data.front() != '{') { // Not JSON
                        if (!is_valid_topic(str_data)) { // Not valid topic
                            cout << "Invalid data received" << endl;
                            continue;
                        }
                        cout << "==> Plain text topic: " << str_data << endl;

                        Distributor target = search_distributor(dist_map, str_data);
                        cout << "target host: " << target.host << endl;
                        cout << "target topic: " << target.topic << endl;

                        if (target.host == "default") {
                            cout << "Distributor object is default" << endl;
                            cout << "Ignore the query" << endl;
                            close(sd);
                            client_socket[i] = 0;
                            continue;
                        }
                        if (target.host == *my_host) {
                            cout << "==> I am Iron Man" << endl;
                            send(sd, target.host.c_str(), strlen(target.host.c_str()), 0 );
                            close(sd);
                            client_socket[i] = 0;
                            continue;
                        }
                        if (target.topic == str_data) {
                            cout << "==> Resolve finish" << endl;
                            send(sd, target.host.c_str(), strlen(target.host.c_str()), 0 );
                            close(sd);
                            client_socket[i] = 0;
                            continue;
                        }
                        //Create socket
                        cout << "==> Create new socket" << endl;
                        int sock = 0;
                        struct sockaddr_in serv_addr;
                        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                            printf("\n Socket creation error \n");
                            return -1;
                        }
                        serv_addr.sin_family = AF_INET;
                        serv_addr.sin_port = htons(PORT);
                        //if(inet_pton(AF_INET, (distr.host).c_str(), &serv_addr.sin_addr)<=0) {
                        if(inet_pton(AF_INET, (target.host).c_str(), &serv_addr.sin_addr)<=0) {
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
                        send_query(sock, str_data);

                        ts_pair = make_pair(str_data, sd); // topic : socket
                        cli_map.insert(ts_pair);
                        continue;
                    }


                    try {
                        read_json(ss_recv, pt);
                    }
                    catch (exception & e) {
                        cerr << "==> Could not parse JSON: " << e.what() << std::endl;
                        continue;
                    }
                    boost::optional<string> type = pt.get_optional<string>("Type");
                    boost::optional<string> topic = pt.get_optional<string>("Topic");
                    boost::optional<string> record = pt.get_optional<string>("Record");
                    boost::optional<string> host= pt.get_optional<string>("Host");
                    if (!type || !topic) {
                        cout << "ERR: Wrong JSON format, cannot parse" << endl;
                        continue;
                    }
                    //if ((*topic).at(0) == '/') {
                    //    cout << "ERR: First character of topic is '/'" << endl;
                    //    continue;
                    //}
                    if (*type == "Hello") {
                        cout << "Received Hello message" << endl;
                        if (!is_valid_topic(*topic)) {
                            cerr << "ERR: Topic begin or finish with '/'" << endl;
                            exit(EXIT_FAILURE);
                        }
                        ptree pt_hello;
                        distr.set_values(*topic, *host, *record);
                        pair = make_pair(distr.topic, distr);
                        dist_map.insert(pair);
                        cout << "ADD Topic: " << *topic << ", Host: " << *host << ", Record: " << *record << endl;

                        // Create hello
                        send_hello(sd, *my_topic, *my_host, *my_record);
                        cout << "Hello message sent" << endl;
                        close(sd);
                        client_socket[i] = 0;
                    } else if (*type == "Query") {

                        Distributor target = search_distributor(dist_map, *topic);

                        if (target.host == "default") {
                            cout << "Distributor object is default" << endl;
                            cout << "Ignore the query" << endl;
                            close(sd);
                            client_socket[i] = 0;
                            continue;
                        }

                        send_reply(sd, *topic, target.topic, target.host, target.record);

                   } else if (*type == "Reply" ) {
                        boost::optional<string> req_topic = pt.get_optional<string>("Requested_topic");
                        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen); 
                        string ip_addr = inet_ntoa(address.sin_addr);
                        // insert pair
                        distr.set_values(*topic, *host, *record);
                        pair = make_pair(*topic, distr);
                        dist_map.insert(pair);
                        cout << "IP address: " << ip_addr << endl;
                        cout << "host: " << *host << endl;
                        if (ip_addr == *host) {
                            cout << "==> Resolve finish" << endl;
                            int cli_sock = cli_map.at(*req_topic);
                            send(cli_sock , (*host).c_str(), strlen((*host).c_str()), 0 );
                            close(sd);
                            cli_map.erase(*topic);
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
                            send_query(sock, *req_topic);
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

