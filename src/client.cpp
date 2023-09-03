#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

enum Error_codes {
  OK, ERROR
};

int main() {
  int errcode = OK;

  // Создаем сокет
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    std::cout << "Failed to create socket" << std::endl;
    errcode = ERROR;
  } else {

    // Устанавливаем адрес сервера и порт
    std::string serverIP = "127.0.0.1";
    int serverPort = 8081;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddr.sin_addr)) <= 0) {
      std::cout << "Invalid address or address not supported" << std::endl;
      close(sockfd);
      errcode = ERROR;
    } else {

      // Устанавливаем соединение с сервером
      if (connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        std::cout << "Connection failed" << std::endl;
        close(sockfd);
        errcode = ERROR;
      } else {
        bool loginDataIsSet = false;
        std::string loginData;


        if (!loginDataIsSet) {
          // Запрос данных для входа на SQL сервер у пользователя
          std::string host, port, dbname, user, password;

          std::cout << "Введите хост: ";
          std::cin >> host;
          std::cout << "Введите порт: ";
          std::cin >> port;
          std::cout << "Введите имя базы данных: ";
          std::cin >> dbname;
          std::cout << "Введите имя пользователя: ";
          std::cin >> user;
          std::cout << "Введите пароль: ";
          std::cin >> password;

          loginData += "host," + host;
          loginData += ",port," + port;
          loginData += ",dbname," + dbname;
          loginData += ",user," + user;
          loginData += ",password," + password;

          loginDataIsSet = true;
        }
        std::string request = loginData;

        // Формируем SQL-запрос на сервер
        std::string query;
        std::cout << "Введите SQL-запрос: ";
        std::getline(std::cin, query);
        if (!query.empty()) {
          query.pop_back();
        }
//        std::cin >> query;
//        request += ",query," + query;

        std::cout << request << std::endl;

        if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
          std::cout << "Failed to send request" << std::endl;
          close(sockfd);
          errcode = ERROR;
        } else {

          // Получаем ответ от сервера
          char buffer[4096];
          memset(buffer, 0, sizeof(buffer));
          std::string response;

          while (true) {
            int bytesRead = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0)
              break;
            response += buffer;
            memset(buffer, 0, sizeof(buffer));
          }

          // Выводим ответ от сервера
          std::cout << "Response from server:" << std::endl;
          std::cout << response << std::endl;

          // Закрываем соединение и освобождаем ресурсы
          close(sockfd);
        }
      }
    }
  }
  return errcode;
}
