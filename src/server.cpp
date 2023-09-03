#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <libpq-fe.h>

#define SERVER_PORT 8080
#define BUFF_SIZE 4096
#define SOCKET_ERROR -1

enum Error_codes {
  OK, ERROR
};

// Функция для логирования SQL-запросов
void logQuery(const std::string &query) {
  // Здесь можно добавить код для записи запроса в лог файл
  std::cout << "Logged query: " << query << std::endl;
}

int main() {
  int errcode = OK;

  // Создание сокета c клиентом
  int proxySocket = socket(AF_INET, SOCK_STREAM, 0);
  if (proxySocket == SOCKET_ERROR) {
    std::cerr << "Failed to create socket" << std::endl;
    errcode = ERROR;
  } else {
    // Настройка адреса сервера
    sockaddr_in proxyAddress{};
    proxyAddress.sin_family = AF_INET;
    proxyAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    proxyAddress.sin_port = htons(SERVER_PORT); // Порт прокси-сервера

    // Привязка сокета к адресу
    if (bind(proxySocket, reinterpret_cast<sockaddr *>(&proxyAddress), sizeof(proxyAddress)) == -1) {
      std::cerr << "Failed to bind socket" << std::endl;
      close(proxySocket);
      errcode = ERROR;
    } else {
      // Прослушивание входящих соединений от клиентов
      if (listen(proxySocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to listen" << std::endl;
        close(proxySocket);
        errcode = ERROR;
      } else {
        std::cout << "Proxy server is running" << std::endl;

        while (true) {
          // Ожидание нового клиентского подключения
          unsigned int clientId = 1;
          sockaddr_in clientAddress{};
          socklen_t clientAddressLength = sizeof(clientAddress);
          int clientSocket = accept(proxySocket, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressLength);
          if (clientSocket == SOCKET_ERROR) {
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
          }

          std::cout << "Accepted new client connection #" << clientId << std::endl;

          // Создание соединения с сервером PostgreSQL
          PGconn *pgConnection = PQconnectdb("host=localhost port=5432 dbname=info_21 user=yonnarge password=1");
          if (PQstatus(pgConnection) != CONNECTION_OK) {
            std::cerr << "Failed to connect to PostgreSQL server: " << PQerrorMessage(pgConnection) << std::endl;
            close(clientSocket);
            continue;
          }

          // Обработка клиентского запроса
          char buffer[BUFF_SIZE];
          ssize_t bytesRead;
          std::string query;

          while ((bytesRead = recv(clientSocket, buffer, BUFF_SIZE - 1, 0)) > 0) {
            buffer[bytesRead] = '\0';
            query += buffer;

            // Проверка наличия завершающего символа запроса
            if (query.find(';') != std::string::npos) {
              // Отправка SQL-запроса на сервер PostgreSQL
              PGresult *pgResult = PQexec(pgConnection, query.c_str());
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
                // Отправка ответа клиенту
                if (send(clientSocket, response.c_str(), response.length(), 0) == SOCKET_ERROR) {
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
            // Закрытие клиентского соединения
            close(clientSocket);
            std::cout << "Close client connection #" << clientId << std::endl;
          }
          // Закрытие соединения с сервером PostgreSQL
          PQfinish(pgConnection);
        }
        // Закрытие прокси-сервера
        close(proxySocket);
      }
    }
  }
  return errcode;
}
