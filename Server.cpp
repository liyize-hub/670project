
#include <sys/socket.h> // socket, bind, listen, inet_ntoa
#include <sys/time.h>   //for gettimeofday()
#include <netinet/in.h> // htonl, htons, inet_ntoa
#include <arpa/inet.h>  // inet_ntoa
#include <netdb.h>      // gethostbyname
#include <unistd.h>     // read, write, close
#include <strings.h>    // bzero
#include <sys/uio.h>    // writev
#include <cstdlib>
#include <iostream>
// #include <cassert>
#include <chrono>
#include <string>
#include <unordered_map>
#include <pthread.h>
#include <fstream>

/*Server.CPP
1. Your server waits for a connection and an HTTP GET request (perform multi-threaded handling).
2. Your server only needs to respond to HTTP GET request.
3. After receiving the GET request if the file exists, the server opens the file that is requested and sends it
(along with the HTTP 200 OK code, of course).
4. If the file requested does not exist,
the server should return a 404 Not Found code along with a custom File Not Found page.
5. If the HTTP request is for SecretFile.html then the web server should return a 401 Unauthorized.
6. If the request is for a file that is above the directory structure where your web server is running
( for example, "GET ../../../etc/passwd" ), you should return a 403 Forbidden.
7. Finally, if your server cannot understand the request, return a 400 Bad Request.
8. After you handle the request, your server should return to waiting for the next request.
*/
using namespace std;

const string OK = "HTTP/1.1 200 OK\r\n";
const string DOES_NOT_EXIST = "HTTP/1.1 404 Not Found\r\n";
const string UNAUTHORIZED = "HTTP/1.1 401 Unauthorized\r\n";
const string FORBIDDEN = "HTTP/1.1 403 Forbidden\r\n";
const string BAD_REQUEST = "HTTP/1.1 400 Bad Request\r\n";

const int PORT = 8888;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

// Struct for Requests given to the server
struct Request
{
    string method;
    string uri;
    string protocol;
};

std::unordered_map<std::string, std::string> mime = {
    {"html", "text/html"},
    {"txt", "text/plain"},
    {"jpg", "image/jpeg"},
    {"gif", " image/gif"}};

string get_file_extension(string s)
{
    int i;
    while (i < s.length())
    {
        if (s[i++] == '.')
        {
            break;
        }
    }
    if (i == s.length())
    {
        return "";
    }
    return s.substr(i, s.length());
}
/**
 * Parsing the data from and incomming request
 * from a client returning a string from the response
 **/
string read_data(int clifd)
{
    string response = "";
    char end = 0;
    while (true)
    {
        char current = 0;
        // receive data from client using read
        recv(clifd, &current, 1, 0);
        if (current == '\n' || current == '\r')
        {
            if (end == '\r' && current == '\n') // to the end '\r\n' in windows
                break;
        }
        else
            response += current;
        end = current;
    }

    return response;
}
/**
 * Once the request header has been read the rest of the request
 * is parsed in order to get the correct file. Returning a
 * request object
 * @param string request
 **/
Request parse_request(string request)
{
    Request parsed;
    string str = "";
    int arg = 0;

    for (int i = 0; i <= request.length(); i++)
    {
        if (arg == 3)
        {
            return parsed;
        }
        if (request[i] == ' ' || i == request.length())
        {
            if (arg == 0)
                parsed.method = str;
            else if (arg == 1)
                parsed.uri = str;
            else if (arg == 2)
                parsed.protocol = str;
            str = "";
            arg++;
        }
        else
            str += request[i];
    }
    return parsed;
}

/**
 * constructing the response Once the request is parsed correctly
 * It is a matter of making sure to respond corrently to the
 * client and send the response back.
 * @param (Request, string, string)
 **/
void contruct_response(Request &request, string &status, string &file_content)
{
    file_content = "";
    // cannot understand request 400
    if (request.method != "GET")
    {
        status = BAD_REQUEST;
        file_content = BAD_REQUEST;
    }
    else if (request.method == "GET")
    {
        // past contains ".." - forbidden 403
        if (request.uri.substr(0, 2) == "..")
        {
            status = FORBIDDEN;
            file_content = FORBIDDEN;
        }
        // request for secret file
        else if (request.uri.length() >= 15 &&
                 request.uri.substr(request.uri.length() - 15, request.uri.length()) == "SecretFile.html")
        {
            status = UNAUTHORIZED;
            file_content = UNAUTHORIZED;
        }
        // request is accepted
        else
        {
            // get current directory using "."
            if (request.uri[0] != '.' && request.uri[0] != '/')
            {
                request.uri = "./" + request.uri;
            }
            else if (request.uri[0] == '/')
            {
                request.uri = "." + request.uri;
            }

            cout << "File: " << request.uri << endl;
            // try to open file, nullptr is does not exist/unathorized
            FILE *file = fopen(request.uri.c_str(), "r");
            if (file == nullptr)
            {
                // no read permission
                if (errno == EACCES)
                {
                    // 401
                    status = UNAUTHORIZED;
                    file_content = UNAUTHORIZED;
                }
                else
                {
                    // 404
                    status = DOES_NOT_EXIST;
                    ifstream file("FileNotFound.txt");
                    string line;
                    if (file.is_open())
                    {
                        while (getline(file, line))
                        {
                            file_content += line;
                        }
                    }
                    file.close();
                }
            }
            else
            {
                // 200
                status = OK;
                while (!feof(file))
                {
                    char c = fgetc(file);
                    if (c < 0)
                    {
                        // char not in ascii table
                        continue;
                    }
                    file_content += c;
                }
                fclose(file);
            }
        }
    }
    else
    {
        status = BAD_REQUEST;
        file_content = BAD_REQUEST;
    }
}

/**
 * Thread function constructs the message that will be sent back to the
 * Retriever
 * @param void *
 **/
void *thread_function(void *arg)
{
    int clifd = *(int *)arg;

    // use port 80 for http
    pthread_mutex_lock(&mutex1);

    // read request
    string request = read_data(clifd);
    cout << "Request: " << request << endl;

    // parse request
    Request parsed = parse_request(request);
    string file_name;
    if (parsed.uri != "")
    {
        file_name = get_file_extension(parsed.uri);
    }
    string content_type;
    if (file_name != "")
    {
        content_type = mime[file_name];
    }

    // construct response
    string code;
    string file_content;
    contruct_response(parsed, code, file_content);
    string pageLength = to_string(file_content.size());
    // 获取当前时间点
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    // 将时间点转化为时间戳
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    string response = code +
                      "Content-Length: " + pageLength + "\r\n" + "Content-Type:" + content_type + "\r\n" +
                      "\r\n" + std::ctime(&now_c) + "\r\n" + file_content;
    cout << response << endl;

    // write response
    int sendResponse = send(clifd, response.c_str(), strlen(response.c_str()), 0);

    pthread_mutex_unlock(&mutex1);
    close(clifd);
    pthread_exit(NULL);
}

/**
 * main function runs commands and begins when script is called
 * @param void *
 **/
int main() // 0 args
{
    // Setup your server's socket address structure
    sockaddr_in serv_addr;
    //  Clear it out, and set its parameters
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;                // use IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // fill in server IP address
    // serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT); // convert port number to network byte order and set port

    // Create your TCP socket
    int serverSD = socket(AF_INET, SOCK_STREAM, 0); // ipv4, connect
    if (serverSD == -1)
    {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return -1;
    }

    const int on = 1;
    int set = setsockopt(serverSD, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(int));

    // Bind socket to IP address and port on server
    bind(serverSD, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    // if (bind(serverSD, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    // {
    //     std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
    //     return -1;
    // }

    cout << "Socket Created" << endl;

    // Listen on the socket for new connections to be made
    // 20 references the size of the queue of outstanding (non-accepted) requests we will store
    if (listen(serverSD, 20) == -1)
    {
        std::cerr << "Error listening to socket: " << strerror(errno) << std::endl;
        return -1;
    }
    int i = 0;

    while (true)
    {
        // create client sd
        sockaddr_in cli_addr;
        socklen_t cli_addrSize = sizeof(cli_addr);

        cout << "Listening on port: " << PORT << endl;
        // accept client
        int clientSD = accept(serverSD, (struct sockaddr *)&cli_addr, &cli_addrSize);
        if (clientSD == -1)
        {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            return -1;
        }
        // assert(clientSD != 0);

        cout << "Client Connected:" << inet_ntoa(cli_addr.sin_addr) << " on port " << ntohs(cli_addr.sin_port) << endl;

        pthread_t thread_id; // create thread
        if (pthread_create(&thread_id, NULL, thread_function, &clientSD) != 0)
        {
            cout << "unable to create thread." << std::endl;
            return -1;
        }
    }
    pthread_exit(NULL);
}