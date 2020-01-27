#include <argon2.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <list>
#include "PasswdMgr.h"
#include "FileDesc.h"
#include "strfuncts.h"
#include <random>
#include <cstdlib>
#include <ctime>
#include <array>

const int hashlen = 32;
const int saltlen = 16;

PasswdMgr::PasswdMgr(const char *pwd_file):_pwd_file(pwd_file) {

}


PasswdMgr::~PasswdMgr() {

}

/*******************************************************************************************
 * checkUser - Checks the password file to see if the given user is listed
 *
 *    Throws: pwfile_error if there were unanticipated problems opening the password file for
 *            reading
 *******************************************************************************************/

bool PasswdMgr::checkUser(const char *name) {
   std::vector<uint8_t> passwd, salt;

   bool result = findUser(name, passwd, salt);

   return result;
     
}

/*******************************************************************************************
 * checkPasswd - Checks the password for a given user to see if it matches the password
 *               in the passwd file
 *
 *    Params:  name - username string to check (case insensitive)
 *             passwd - password string to hash and compare (case sensitive)
 *    
 *    Returns: true if correct password was given, false otherwise
 *
 *    Throws: pwfile_error if there were unanticipated problems opening the password file for
 *            reading
 *******************************************************************************************/

bool PasswdMgr::checkPasswd(const char *name, const char *passwd) {
   std::vector<uint8_t> userhash; // hash read from the password file
   std::vector<uint8_t> passhash; // new hash to be generated from entered passwrd
   std::vector<uint8_t> salt; //salt read from the password file

   // Check if the user exists and get the hashed password / salt
   if (!findUser(name, userhash, salt))
      return false;

   //Hash the entered password using the salt
   hashArgon2(passhash, salt, passwd, &salt);

   //Compare the two hash values
   if (userhash == passhash)
      return true;

   return false;
}

/*******************************************************************************************
 * changePasswd - Changes the password for the given user to the password string given
 *
 *    Params:  name - username string to change (case insensitive)
 *             passwd - the new password (case sensitive)
 *
 *    Returns: true if successful, false if the user was not found
 *
 *    Throws: pwfile_error if there were unanticipated problems opening the password file for
 *            writing
 *
 *******************************************************************************************/

bool PasswdMgr::changePasswd(const char *name, const char *passwd) {

   // Insert your insane code here

   return true;
}

/*****************************************************************************************************
 * readUser - Taking in an opened File Descriptor of the password file, reads in a user entry and
 *            loads the passed in variables
 *
 *    Params:  pwfile - FileDesc of password file already opened for reading
 *             name - std string to store the name read in
 *             hash, salt - vectors to store the read-in hash and salt respectively
 *
 *    Returns: true if a new entry was read, false if eof reached 
 * 
 *    Throws: pwfile_error exception if the file appeared corrupted
 *
 *****************************************************************************************************/

bool PasswdMgr::readUser(FileFD &pwfile, std::string &name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt)
{
   std::vector<uint8_t> newline;
   ssize_t eof_check;
   //Readline from the pwfile and write the value into the &name variable
   //Should return 0 if it read EOF
   if (!pwfile.readStr(name)) {
      return false;
   }
   pwfile.readBytes(hash, hashlen);
   pwfile.readBytes(salt, saltlen);
   pwfile.readBytes(newline, 1);
   return true;
}

/*****************************************************************************************************
 * writeUser - Taking in an opened File Descriptor of the password file, writes a user entry to disk
 *
 *    Params:  pwfile - FileDesc of password file already opened for writing
 *             name - std string of the name 
 *             hash, salt - vectors of the hash and salt to write to disk
 *
 *    Returns: bytes written
 *
 *    Throws: pwfile_error exception if the writes fail
 *
 *****************************************************************************************************/

int PasswdMgr::writeUser(FileFD &pwfile, std::string &name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt)
{
   int results = 0;

   //Append a newline and write the name to the file
   name += '\n';
   pwfile.writeFD(name);
   results += sizeof(name);

   //Write the hash and salt to the file
   pwfile.writeBytes<uint8_t>(hash);
   results += sizeof(hash);
   
   pwfile.writeBytes<uint8_t>(salt);
   results += sizeof(salt);

   //Add a terminating newline
   pwfile.writeFD("\n");
   results += 1;

   return results;
}

/*****************************************************************************************************
 * findUser - Reads in the password file, finding the user (if they exist) and populating the two
 *            passed in vectors with their hash and salt
 *
 *    Params:  name - the username to search for
 *             hash - vector to store the user's password hash
 *             salt - vector to store the user's salt string
 *
 *    Returns: true if found, false if not
 *
 *    Throws: pwfile_error exception if the pwfile could not be opened for reading
 *
 *****************************************************************************************************/

bool PasswdMgr::findUser(const char *name, std::vector<uint8_t> &hash, std::vector<uint8_t> &salt) {
   FileFD pwfile(_pwd_file.c_str());

   // You may need to change this code for your specific implementation
   if (!pwfile.openFile(FileFD::readfd))
      throw pwfile_error("Could not open passwd file for reading");

   // Password file should be in the format username\n{32 byte hash}{16 byte salt}\n
   bool eof = false;
   while (!eof) {
      std::string uname;

      if (!readUser(pwfile, uname, hash, salt)) {
         eof = true;
         continue;
      }

      if (!uname.compare(name)) {
         pwfile.closeFD();
         return true;
      }
   }

   hash.clear();
   salt.clear();
   pwfile.closeFD();
   return false;
}


/*****************************************************************************************************
 * hashArgon2 - Performs a hash on the password using the Argon2 library. Implementation algorithm
 *              taken from the http://github.com/P-H-C/phc-winner-argon2 example. 
 *
 *    Params:  dest - the std string object to store the hash
 *             passwd - the password to be hashed
 *
 *    Throws: runtime_error if the salt passed in is not the right size
 *****************************************************************************************************/
void PasswdMgr::hashArgon2(std::vector<uint8_t> &ret_hash, std::vector<uint8_t> &ret_salt, 
                           const char *in_passwd, std::vector<uint8_t> *in_salt) {
   // Hash those passwords!!!!
    uint32_t pwdlen = sizeof(in_passwd);
    std::array<uint8_t,hashlen> hash1;
    std::array<uint8_t,saltlen> salt;

    uint32_t t_cost = 2;            // 1-pass computation
    uint32_t m_cost = (1<<16);      // 64 mebibytes memory usage
    uint32_t parallelism = 1;       // number of threads and lanes

   for (int i = 0; i < saltlen; i++) {
      salt[i] = ret_salt[i];
   }
    // high-level API
   argon2i_hash_raw(t_cost, m_cost, parallelism, in_passwd, pwdlen, salt.data(), saltlen, hash1.data(), hashlen);
   for (auto i = hash1.begin(); i != hash1.end(); ++i) {
      ret_hash.push_back(*i);
   }
}

/****************************************************************************************************
 * addUser - First, confirms the user doesn't exist. If not found, then adds the new user with a new
 *           password and salt
 *
 *    Throws: pwfile_error if issues editing the password file
 ****************************************************************************************************/

void PasswdMgr::addUser(const char *name, const char *passwd) {
   //Need to generate an entry in the passwd file using a randomly generated 16byte hash and the provided username/passwd
   std::vector<uint8_t> passhash; // buffer to store the new hash
   std::vector<uint8_t> salt; //buffer to store the new salt

   //Generate Salt
   generateSalt(salt);

   //Hash the salt + password
   hashArgon2(passhash, salt, passwd, &salt);

   //Open the password file
   FileFD pwfile(_pwd_file.c_str());
   if (!pwfile.openFile(FileFD::appendfd))
      throw pwfile_error("Could not open passwd file for writing");

   //Call writeUser function to write new entry to the openfile
   std::string stringName = name;
   writeUser(pwfile, stringName, passhash, salt);

}

uint8_t PasswdMgr::genRandom()  // Random string generator function.
{
   //salt alphabet
   static const char alphanum[] =
   "0123456789"
   "!@#$%^&*"
   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
   "abcdefghijklmnopqrstuvwxyz";
   
   int stringLength = sizeof(alphanum) - 1;
   //random char generation
   return alphanum[rand() % stringLength];
}

void PasswdMgr::generateSalt(std::vector<uint8_t> &in_salt) {

   //Populate vector with 16 random alphanumber characters
   srand(time(0));
    for(int z=0; z < saltlen; z++)
    {
        in_salt.push_back(genRandom());
    }

}



