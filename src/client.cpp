#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

void saveLoginDataToFile() {
  std::string host, port, database, user, password;

  // Запрос данных у пользователя
  std::cout << "Введите хост: ";
  std::getline(std::cin, host);

  std::cout << "Введите порт: ";
  std::getline(std::cin, port);

  std::cout << "Введите имя базы данных: ";
  std::getline(std::cin, database);

  std::cout << "Введите имя пользователя: ";
  std::getline(std::cin, user);

  std::cout << "Введите пароль: ";
  std::getline(std::cin, password);

  // Открытие файла для записи
  std::ofstream file("login_data.txt");
  if (!file.is_open()) {
    std::cout << "Не удалось открыть файл для записи." << std::endl;
  } else {
    // Запись данных в файл
    file << "host," << host << std::endl;
    file << "port," << port << std::endl;
    file << "database," << database << std::endl;
    file << "user," << user << std::endl;
    file << "password," << password << std::endl;

    // Закрытие файла
    file.close();

    std::cout << "Данные успешно сохранены в файл login_data.txt." << std::endl;
  }
}

void sendFileToServer(const std::string& filename, const std::string& serverIP, int serverPort) {
  // Открытие файла для чтения
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cout << "Failed to open file" << std::endl;
    return;
  }

  // Получение размера файла
  file.seekg(0, std::ios::end);
  std::streampos fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  // Чтение содержимого файла в буфер
  std::vector<char> fileContent(fileSize);
  file.read(fileContent.data(), fileSize);

  // Создание сокета
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    std::cout << "Failed to create socket" << std::endl;
    return;
  }

  // Установка адреса сервера и порта
  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(serverPort);
  if (inet_pton(AF_INET, serverIP.c_str(), &(serverAddr.sin_addr)) <= 0) {
    std::cout << "Invalid address or address not supported" << std::endl;
    close(sockfd);
    return;
  }

  // Установка соединения с сервером
  if (connect(sockfd, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
    std::cout << "Connection failed" << std::endl;
    close(sockfd);
    return;
  }

  // Отправка размера файла на сервер
  if (send(sockfd, &fileSize, sizeof(fileSize), 0) < 0) {
    std::cout << "Failed to send file size" << std::endl;
    close(sockfd);
    return;
  }

  // Отправка содержимого файла на сервер
  if (send(sockfd, fileContent.data(), fileSize, 0) < 0) {
    std::cout << "Failed to send file content" << std::endl;
    close(sockfd);
    return;
  }

  // Закрытие соединения и освобождение ресурсов
  close(sockfd);
}

int main() {
  // Создаем сокет
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    std::cout << "Failed to create socket" << std::endl;
    return 1;
  }

  std::string filename = "login_data.txt";
  sendFileToServer(filename, serverIP, serverPort);

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
