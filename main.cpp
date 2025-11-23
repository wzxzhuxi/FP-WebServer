#include "parser/http_parser.hpp"
#include <iostream>

int main() {
    std::string_view raw_request =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:9006\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    auto result = http::parser::parse_http_request(raw_request);

    if (result) {
        const auto& req = *result;

        std::cout << "Method: "
                  << (req.request_line.method == http::parser::Method::Get
                      ? "GET" : "OTHER")
                  << "\n";
        std::cout << "URI: " << req.request_line.uri << "\n";
        std::cout << "Version: "
                  << (req.request_line.version == http::parser::Version::Http11
                      ? "HTTP/1.1" : "HTTP/1.0")
                  << "\n";

        std::cout << "Headers:\n";
        for (const auto& [key, value] : req.headers) {
            std::cout << "  " << key << ": " << value << "\n";
        }
    } else {
        std::cerr << "Parse error!\n";
    }

    return 0;
}
