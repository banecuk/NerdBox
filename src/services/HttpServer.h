#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WebServer.h>

class HttpServer {
   public:
    HttpServer();
    void begin();
    void processRequests();

   private:
    WebServer server_;

    void handleNotFound();
    void handleHome();
    void handleSystemInfo();

    String getSystemInfo();

    String wrapHtmlContent(const String &title, const String &content);
};

#endif  // HTTP_SERVER_H