#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <libpq-fe.h>

// Функция для логирования SQL-запросов
void logQuery(const std::string& query) {
  // Здесь можно добавить код для записи запроса в лог файл
  std::cout << "Logged query: " << query << std::endl;
}

int main() {
  // Создание сокета
  int proxySocket = socket(AF_INET, SOCK_STREAM, 0);
  if (proxySocket == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return 1;
  }

  // Настройка адреса сервера
  sockaddr_in proxyAddress{};
  proxyAddress.sin_family = AF_INET;
  proxyAddress.sin_addr.s_addr = htonl(INADDR_ANY);
  proxyAddress.sin_port = htons(8080); // Порт прокси-сервера

  // Привязка сокета к адресу
  if (bind(proxySocket, reinterpret_cast<sockaddr*>(&proxyAddress), sizeof(proxyAddress)) == -1) {
    std::cerr << "Failed to bind socket" << std::endl;
    close(proxySocket);
    return 1;
  }

  // Прослушивание входящих соединений
  if (listen(proxySocket, SOMAXCONN) == -1) {
    std::cerr << "Failed to listen" << std::endl;
    close(proxySocket);
    return 1;
  }

  std::cout << "Proxy server is running" << std::endl;

  while (true) {
    // Ожидание нового клиентского подключения
    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket = accept(proxySocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
    if (clientSocket == -1) {
      std::cerr << "Failed to accept client connection" << std::endl;
      continue;
    }

    std::cout << "Accepted new client connection" << std::endl;

    // Создание соединения с сервером PostgreSQL
    PGconn* pgConnection = PQconnectdb("host=localhost port=5432 dbname=info_21 user=yonnarge password=1");
    if (PQstatus(pgConnection) != CONNECTION_OK) {
      std::cerr << "Failed to connect to PostgreSQL server: " << PQerrorMessage(pgConnection) << std::endl;
      close(clientSocket);
      continue;
    }

    // Обработка клиентского запроса
    char buffer[4096];
    ssize_t bytesRead;
    std::string query;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
      buffer[bytesRead] = '\0';
      query += buffer;

      // Проверка наличия завершающего символа запроса
      if (query.find(';') != std::string::npos) {
        // Отправка SQL-запроса на сервер PostgreSQL
        PGresult* pgResult = PQexec(pgConnection, query.c_str());
        if (PQresultStatus(pgResult) == PGRES_COMMAND_OK || PQresultStatus(pgResult) == PGRES_TUPLES_OK) {
          // Логирование SQL-запроса
          logQuery(query);

          // Пример формирования ответа на основе результата SQL-запроса
          std::string response = "Result:\n";
          int row_count = PQntuples(pgResult);
          int col_count = PQnfields(pgResult);

          for (int row = 0; row < row_count; ++row) {
            for (int col = 0; col < col_count; ++col) {
              response += PQgetvalue(pgResult, row, col);
              response += "\t";
            }
            response += "\n";
          }

          std::cout << response;
          // Отправка ответа клиенту
          if (send(clientSocket, response.c_str(), response.length(), 0) == -1) {
            std::cerr << "Failed to send response to client" << std::endl;
            break;
          }

        } else {
          std::cerr << "Failed to execute SQL query: " << PQresultErrorMessage(pgResult) << std::endl;
        }

        // Очистка результата запроса
        PQclear(pgResult);

        // Очистка буфера запроса
        query.clear();
      }

      // Отправка ответа клиенту
      if (send(clientSocket, buffer, bytesRead, 0) == -1) {
        std::cerr << "Failed to send response to client" << std::endl;
        break;
      }
    }

    // Закрытие клиентского соединения
    close(clientSocket);

    // Закрытие соединения с сервером PostgreSQL
    PQfinish(pgConnection);
  }

  // Закрытие прокси-сервера
  close(proxySocket);

  return 0;
}
