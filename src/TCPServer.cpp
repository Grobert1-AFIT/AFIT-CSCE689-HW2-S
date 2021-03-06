#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <strings.h>
#include <vector>
#include <iostream>
#include <memory>
#include <sstream>
#include "TCPServer.h"
#include <fstream>
#include <algorithm>

TCPServer::TCPServer(){ 
   logServer = std::make_shared<LogSvr>("server.log");
}


TCPServer::~TCPServer() {

}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {

   struct sockaddr_in servaddr;

   logServer->logString("Server started @ ");

   // Set the socket to nonblocking
   _sockfd.setNonBlocking();

   // Load the socket information to prep for binding
   _sockfd.bindFD(ip_addr, port);

   //Populate whitelist for incoming connections
   std::ifstream wlFile;
   std::string line;

   wlFile.open("whitelist");
   if (!wlFile) {
      std::cout << "Unable to read Whitelist file\n";
   }
   else {
      while (getline(wlFile, line)) {
         //add line to the vector
         whiteList.push_back(line);
      }
   }
   wlFile.close();

}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::listenSvr() {

   bool online = true;
   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 100000000;
   int num_read = 0;

   // Start the server socket listening
   _sockfd.listenFD(5);

    
   while (online) {
      struct sockaddr_in cliaddr;
      socklen_t len = sizeof(cliaddr);

      if (_sockfd.hasData()) {
         TCPConn *new_conn = new TCPConn(logServer);
         if (!new_conn->accept(_sockfd)) {
            // _server_log.strerrLog("Data received on socket but failed to accept.");
            continue;
         }
         std::cout << "***Got a connection***\n";

         // Get their IP Address string to use in logging and check if they are on the White-List
         std::string ipaddr_str;
         new_conn->getIPAddrStr(ipaddr_str);

         //Connection IP Matches WhiteList do the normal stuff
         if (std::find(whiteList.begin(), whiteList.end(), ipaddr_str) != whiteList.end()) {

            _connlist.push_back(std::unique_ptr<TCPConn>(new_conn));

            new_conn->sendText("Welcome to the CSCE 689 Server!\n");

            //Log the event
            logServer->logString("Connection from " + ipaddr_str + "@ ");

            // Change this later
            new_conn->startAuthentication();
         }
         //Unauthorized IP disconnect the connection
         else {
            new_conn->sendText("Unauthorized Connection, disconnecting!\n");
            new_conn->disconnect();
            //Log the event
            logServer->logString("Unauthorized connection attempt from" + ipaddr_str + "@ ");

         }
      }

      // Loop through our connections, handling them
      std::list<std::unique_ptr<TCPConn>>::iterator tptr = _connlist.begin();
      while (tptr != _connlist.end())
      {
         // If the user lost connection
         if (!(*tptr)->isConnected()) {
            // Log it

            // Remove them from the connect list
            tptr = _connlist.erase(tptr);
            std::cout << "Connection disconnected.\n";
            continue;
         }

         // Process any user inputs
         (*tptr)->handleConnection();

         // Increment our iterator
         tptr++;
      }

      // So we're not chewing up CPU cycles unnecessarily
      nanosleep(&sleeptime, NULL);
   } 


   
}


/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() {

   _sockfd.closeFD();
}


