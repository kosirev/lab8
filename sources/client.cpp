
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

int main(int argc, char** argv)     //выполняет http запрос и выводит ответ
{
  try {
    // Check command line arguments.
    if (argc != 5 && argc != 6) {
      std::cerr <<
                "Usage: http-client-sync <host> <port> <target> <body>"
          "[<HTTP version: 1.0 or 1.1(default)>]\n" <<
                "Example:\n" <<
                "    http-client-sync www.example.com 80 / hello\n" <<
                "    http-client-sync www.example.com 80 / hello 1.0\n";
      return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const target = argv[3];
    auto const body = argv[4];
    int version = argc == 6 && !std::strcmp("1.0", argv[5]) ? 10 : 11;

    net::io_context ioc;                //необходим для всех операций ввода-вывода

    tcp::resolver resolver(ioc);    //Эти объекты выполняют наш ввод-вывод
    beast::tcp_stream stream(ioc);

    auto const results = resolver.resolve(host, port); //поиск имя домена


    stream.connect(results);  //Установите соединение по IP-адресу, который мы получаем из поиска

    //Настройка сообщения запроса HTTP GET
    http::request<http::string_body> req{http::verb::post, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");
    req.body() = "{\n\t\"input\": \"" + std::string(body) + "\"\n}";

    req.prepare_payload();
    // Отправить HTTP-запрос на удаленный хост
    http::write(stream, req);

    // Этот буфер используется для чтения и должен сохраняться
    beast::flat_buffer buffer;

    //Объявите контейнер для хранения ответа
    http::response<http::dynamic_body> res;

    // Получить HTTP-ответ
    http::read(stream, buffer, res);

    // Напишите сообщение в standard out
    std::cout << res << std::endl;

    // Изящно закройте сокет
    beast::error_code ec;
    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    // not_connected иногда случается
    // так что не утруждайте себя сообщением об этом.

    if (ec && ec != beast::errc::not_connected)
      throw beast::system_error{ec};

    // Если мы доберемся сюда, то соединение будет закрыто изящно
  } catch(std::exception const& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
