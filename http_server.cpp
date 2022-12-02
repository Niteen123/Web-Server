#include "http_server.hh"
#include <vector>
#include <sys/stat.h>
#include <sys/dir.h>
#include <fstream>
#include <sstream>

vector<string> split(const string &s, char delim)
{
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim))
  {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request)
{
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

  /*
   TODO : extract the request method and URL from first_line here
  */
  this->method = first_line[0];
  this->url = first_line[1];

  if (this->method != "GET")
  {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}

HTTP_Response *handle_request(string req)
{
  HTTP_Request *request = new HTTP_Request(req);

  HTTP_Response *response = new HTTP_Response();

  string url = string("html_files") + request->url;

  response->HTTP_version = "1.0";

  struct stat sb;
  string file_name;
  DIR *directory;
  file_name.clear();
  if (stat(url.c_str(), &sb) == 0) // requested path exists
  {
    string body;

    if (S_ISDIR(sb.st_mode))
    {
      /*
      In this case, requested path is a directory.
      TODO : find the index.html file in that directory (modify the url
      accordingly)
      */
      if ((directory = opendir(url.c_str())) == NULL)
      {
        perror("error while opening the directory");
        // url = url + "/" + "index.html";
      }
      else
      {
        url = url + "/" + "index.html";
        // cout<<url;
      }
      closedir(directory);
    }
    /*
    TODO : open the file and read its contents
    */
    string line;
    fstream read_file;
    read_file.open(url);
    int length = 0;

    while (getline(read_file, line))
    {
      length = length + line.length();
      response->body.append(line);
    }
    read_file.close();
    /*
    TODO : set the remaining fields of response appropriately
    */
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";
    response->content_length = to_string(length);
  }
  else
  {

    string error = "<html><head><title>404 not found</title></head><body>ERROR 404 Not Found</body></html>";
    int length = error.length();

    response->content_length = to_string(length);
    response->body.append(error);
    response->status_code = "404";
    response->status_text = "Not Found";
    response->content_type = "text/html";
      /*
      TODO : set the remaining fields of response appropriately
      */
  }

  delete request;

  return response;
}

string HTTP_Response::get_string()
{
  /*
  TODO : implement this function
  */
  char buf[1000];
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z \r\n", &tm);
  string result = "";
  string HTTP_version = "HTTP/1.1 ";
  string status_code = this->status_code + " ";
  string status_text = this->status_text + "\r\n";
  string content_type = "Content-Type: " + this->content_type + "\r\n";
  string content_length = "Content-Length: " + this->content_length + "\r\n\n";
  result = HTTP_version + status_code + status_text + "DATE: " + buf + content_type + content_length + body;
  cout << "\n" + result << endl;
  return result;
}
