#include <stdexcept>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "TCPConn.h"
#include "strfuncts.h"
#include "PasswdMgr.h"

// The filename/path of the password file
const char pwdfilename[] = "passwd";

//Need to make a PasswdMgr to handle your username/password functions

TCPConn::TCPConn(std::shared_ptr<LogSvr> inputServer):pwdMgr(pwdfilename) { // LogMgr &server_log):_server_log(server_log) {
   logServer = inputServer;
}


TCPConn::~TCPConn() {

}

/**********************************************************************************************
 * accept - simply calls the acceptFD FileDesc method to accept a connection on a server socket.
 *
 *    Params: server - an open/bound server file descriptor with an available connection
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

bool TCPConn::accept(SocketFD &server) {
   return _connfd.acceptFD(server);
}

/**********************************************************************************************
 * sendText - simply calls the sendText FileDesc method to send a string to this FD
 *
 *    Params:  msg - the string to be sent
 *             size - if we know how much data we should expect to send, this should be populated
 *
 *    Throws: runtime_error for unrecoverable errors
 **********************************************************************************************/

int TCPConn::sendText(const char *msg) {
   return sendText(msg, strlen(msg));
}

int TCPConn::sendText(const char *msg, int size) {
   if (_connfd.writeFD(msg, size) < 0) {
      return -1;  
   }
   return 0;
}

/**********************************************************************************************
 * startAuthentication - Sets the status to request username
 *
 *    Throws: runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPConn::startAuthentication() {

   //Sets status of connection to username
   _status = s_username;

   _connfd.writeFD("Username: "); 

}

/**********************************************************************************************
 * handleConnection - performs a check of the connection, looking for data on the socket and
 *                    handling it based on the _status, or stage, of the connection
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::handleConnection() {

   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 100000000;

   try {
      switch (_status) {
         case s_username:
            getUsername();
            break;

         case s_passwd:
            getPasswd();
            break;
   
         case s_changepwd:
         case s_confirmpwd:
            changePassword();
            break;

         case s_menu:
            getMenuChoice();

            break;

         default:
            throw std::runtime_error("Invalid connection status!");
            break;
      }
   } catch (socket_error &e) {
      std::cout << "Socket error, disconnecting.";
      disconnect();
      return;
   }

   nanosleep(&sleeptime, NULL);
}

/**********************************************************************************************
 * getUsername - called from handleConnection when status is s_username--if it finds user data,
 *               it expects a username and compares it against the password database
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::getUsername() {
   //Wait for user data to continue
   if (!_connfd.hasData())
      return;
   std::string username;


   //username should be populated with user input
   if (!getUserInput(username))
      return;


   //Check username list for username entered
      std::vector<uint8_t> hash, salt;
      if (pwdMgr.checkUser(username.c_str())) {
         _status = s_passwd;
         _username = username;
         _connfd.writeFD("Password: ");
      }
      //No matching username
      else {
         _connfd.writeFD("Invalid Username, disconnecting...");
         disconnect();

         //Log the event
         std::string ip;
         _connfd.getIPAddrStr(ip);
         logServer->logString("Invalid username entered - " + username + " from " + ip + " @ " );

      }
}

/**********************************************************************************************
 * getPasswd - called from handleConnection when status is s_passwd--if it finds user data,
 *             it assumes it's a password and hashes it, comparing to the database hash. Users
 *             get two tries before they are disconnected
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::getPasswd() {
   //Wait on user input
   if (!_connfd.hasData())
      return;


   std::string password;
   bool authenticated = false;
   int attempts = 0;


   while (!authenticated) {
      //Read the password from client
      if (!getUserInput(password))
         return;

      //Check if the password matches the stored one
      if (pwdMgr.checkPasswd(_username.c_str(), password.c_str())) {
         _status = s_menu;
         sendMenu();
         authenticated = true;
         
         //Log the event
         std::string ip;
         _connfd.getIPAddrStr(ip);
         logServer->logString(_username + " from " + ip + " successfully authenticated @ ");
      }

      //Incorrect password attempt
      else { _connfd.writeFD("Incorrect Password try again.\n"); }
      attempts += 1; 

      //Too many incorrect attempts
      if (attempts == max_attempts) {
         _connfd.writeFD("Too many unsuccessful attempts, disconnecting...");
         _connfd.closeFD();

         //Log the event
         std::string ip;
         _connfd.getIPAddrStr(ip);
         logServer->logString(_username + " from " + ip + " unsuccessfully authenticated @ ");


         return;
      }
   }
}

/**********************************************************************************************
 * changePassword - called from handleConnection when status is s_changepwd or s_confirmpwd--
 *                  if it finds user data, with status s_changepwd, it saves the user-entered
 *                  password. If s_confirmpwd, it checks to ensure the saved password from
 *                  the s_changepwd phase is equal, then saves the new pwd to the database
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::changePassword() {
   //Wait on user input
   if (!_connfd.hasData())
      return;

   //switch on status of first password or second
   switch(_status) {

      //First entry
      case s_changepwd:
         //Read the command line for new password
         if (!getUserInput(_newpwd))
            return;
         _status = s_confirmpwd;
         _connfd.writeFD("Confirm Password: ");
         return;

      //Second entry
      case s_confirmpwd:
         std::string confirmPwd;
         if (!getUserInput(confirmPwd))
            return;
         if (confirmPwd !=_newpwd) {
            //Passwords don't match return them to menu
            _connfd.writeFD("Passwords do not match, aborting...\n");
            _status = s_menu;
            sendMenu();
            _newpwd.clear();
            return;
         }
         else {
            //Passwords matched, need to record the new password
            pwdMgr.changePasswd(_username.c_str(), _newpwd.c_str());
            _status = s_menu;
            sendMenu();
            //Clear out the stored password
            _newpwd.clear();
         }

   }


}


/**********************************************************************************************
 * getUserInput - Gets user data and includes a buffer to look for a carriage return before it is
 *                considered a complete user input. Performs some post-processing on it, removing
 *                the newlines
 *
 *    Params: cmd - the buffer to store commands - contents left alone if no command found
 *
 *    Returns: true if a carriage return was found and cmd was populated, false otherwise.
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

bool TCPConn::getUserInput(std::string &cmd) {
   std::string readbuf;

   // read the data on the socket
   _connfd.readFD(readbuf);

   // concat the data onto anything we've read before
   _inputbuf += readbuf;

   // If it doesn't have a carriage return, then it's not a command
   int crpos;
   if ((crpos = _inputbuf.find("\n")) == std::string::npos)
      return false;

   cmd = _inputbuf.substr(0, crpos);
   _inputbuf.erase(0, crpos+1);

   // Remove \r if it is there
   clrNewlines(cmd);

   return true;
}

/**********************************************************************************************
 * getMenuChoice - Gets the user's command and interprets it, calling the appropriate function
 *                 if required.
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

void TCPConn::getMenuChoice() {
   if (!_connfd.hasData())
      return;
   std::string cmd;
   if (!getUserInput(cmd))
      return;
   lower(cmd);      

   // Don't be lazy and use my outputs--make your own!
   std::string msg;
   if (cmd.compare("hello") == 0) {
      _connfd.writeFD("Hello back!\n");
   } else if (cmd.compare("menu") == 0) {
      sendMenu();
   } else if (cmd.compare("exit") == 0) {
      _connfd.writeFD("Disconnecting...goodbye!\n");
      disconnect();
   } else if (cmd.compare("passwd") == 0) {
      _connfd.writeFD("New Password: ");
      _status = s_changepwd;
   } else if (cmd.compare("1") == 0) {
      msg += "You want a prediction about the weather? You're asking the wrong Phil.\n";
      msg += "I'm going to give you a prediction about this winter. It's going to be\n";
      msg += "cold, it's going to be dark and it's going to last you for the rest of\n";
      msg += "your lives!\n";
      _connfd.writeFD(msg);
   } else if (cmd.compare("2") == 0) {
      _connfd.writeFD("42\n");
   } else if (cmd.compare("3") == 0) {
      _connfd.writeFD("That seems like a terrible idea.\n");
   } else if (cmd.compare("4") == 0) {

   } else if (cmd.compare("5") == 0) {
      _connfd.writeFD("I'm singing, I'm in a computer and I'm siiiingiiiing! I'm in a\n");
      _connfd.writeFD("computer and I'm siiiiiiinnnggiiinnggg!\n");
   } else {
      msg = "Unrecognized command: ";
      msg += cmd;
      msg += "\n";
      _connfd.writeFD(msg);
   }

}

/**********************************************************************************************
 * sendMenu - sends the menu to the user via their socket
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
void TCPConn::sendMenu() {
   std::string menustr;

   // Make this your own!
   menustr += "Available choices: \n";
   menustr += "  1). Provide weather report.\n";
   menustr += "  2). Learn the secret of the universe.\n";
   menustr += "  3). Play global thermonuclear war\n";
   menustr += "  4). Do nothing.\n";
   menustr += "  5). Sing. Sing a song. Make it simple, to last the whole day long.\n\n";
   menustr += "Other commands: \n";
   menustr += "  Hello - self-explanatory\n";
   menustr += "  Passwd - change your password\n";
   menustr += "  Menu - display this menu\n";
   menustr += "  Exit - disconnect.\n\n";

   _connfd.writeFD(menustr);
}


/**********************************************************************************************
 * disconnect - cleans up the socket as required and closes the FD
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
void TCPConn::disconnect() {

   //Log the event
   std::string ip;
   _connfd.getIPAddrStr(ip);
   if (_username.length() != 0) {
      logServer->logString(_username + " from " + ip + " disconnected @ ");
   }
   //Disconnected before valid username
   else {
      logServer->logString("Connection from " + ip + " disconnected @ ");
   }
   _connfd.closeFD();
}


/**********************************************************************************************
 * isConnected - performs a simple check on the socket to see if it is still open 
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
bool TCPConn::isConnected() {
   return _connfd.isOpen();
}

/**********************************************************************************************
 * getIPAddrStr - gets a string format of the IP address and loads it in buf
 *
 **********************************************************************************************/
void TCPConn::getIPAddrStr(std::string &buf) {
   return _connfd.getIPAddrStr(buf);
}

