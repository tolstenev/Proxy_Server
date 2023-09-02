#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
  // Создаем сокет
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    std::cout << "Failed to create socket" << std::endl;
    return 1;
  }

  // Устанавливаем адрес сервера и порт
  std::string serverIP = "127.0.0.1";
  int serverPort = 8080;

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(serverPort);
  if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddr.sin_addr)) <= 0) {
    std::cout << "Invalid address or address not supported" << std::endl;
    close(sockfd);
    return 1;
  }

  // Устанавливаем соединение с сервером
  if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
    std::cout << "Connection failed" << std::endl;
    close(sockfd);
    return 1;
  }

  // Отправляем SQL-запрос на сервер
  std::string sqlQuery = "SELECT * FROM peers";
  std::string request = sqlQuery + ";";
  if (send(sockfd, request.c_str(), request.length(), 0) < 0) {
    std::cout << "Failed to send SQL query" << std::endl;
    close(sockfd);
    return 1;
  }

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

  return 0;
}
