#include <iostream>
#include <string>
#include <pqxx/pqxx>


void addAuthor(pqxx::connection* conn) {
    std::string name;
    int year;

    std::cout << "Имя автора: ";
    std::getline(std::cin, name);
    std::cout << "Год рождения: ";
    std::cin >> year;
    std::cin.ignore();

    pqxx::work txn(*conn);
    txn.exec("INSERT INTO authors (name, birth_year) VALUES (" +
        txn.quote(name) + ", " + txn.quote(year) + ")");
    txn.commit();

    std::cout << "Автор добавлен\n";
}

void addBook(pqxx::connection* conn) {
    std::string title, genre;
    int author_id, year;

    std::cout << "Название: ";
    std::getline(std::cin, title);
    std::cout << "ID автора: ";
    std::cin >> author_id;
    std::cout << "Год издания: ";
    std::cin >> year;
    std::cin.ignore();
    std::cout << "Жанр: ";
    std::getline(std::cin, genre);

    pqxx::work txn(*conn);
    txn.exec("INSERT INTO books (title, author_id, year, genre) VALUES (" +
        txn.quote(title) + ", " + txn.quote(author_id) + ", " +
        txn.quote(year) + ", " + txn.quote(genre) + ")");
    txn.commit();

    std::cout << "Книга добавлена\n";
}

void addUser(pqxx::connection* conn) {
    std::string name;

    std::cout << "Имя пользователя: ";
    std::getline(std::cin, name);

    pqxx::work txn(*conn);
    txn.exec("INSERT INTO users (name) VALUES (" + txn.quote(name) + ")");
    txn.commit();

    std::cout << "Пользователь добавлен\n";
}

void borrowBook(pqxx::connection* conn) {
    int user_id, book_id;

    std::cout << "ID пользователя: ";
    std::cin >> user_id;
    std::cout << "ID книги: ";
    std::cin >> book_id;
    std::cin.ignore();

    pqxx::work check(*conn);
    auto result = check.exec("SELECT * FROM borrowed WHERE book_id = " +
        check.quote(book_id) + " AND return_date IS NULL");
    check.commit();

    if (!result.empty()) {
        std::cout << "Книга уже взята\n";
        return;
    }

    pqxx::work txn(*conn);
    txn.exec("INSERT INTO borrowed (user_id, book_id) VALUES (" +
        txn.quote(user_id) + ", " + txn.quote(book_id) + ")");
    txn.commit();

    std::cout << "Книга взята\n";
}

void returnBook(pqxx::connection* conn) {
    int user_id, book_id;

    std::cout << "ID пользователя: ";
    std::cin >> user_id;
    std::cout << "ID книги: ";
    std::cin >> book_id;
    std::cin.ignore();

    pqxx::work txn(*conn);
    txn.exec("UPDATE borrowed SET return_date = CURRENT_DATE "
        "WHERE user_id = " + txn.quote(user_id) +
        " AND book_id = " + txn.quote(book_id) +
        " AND return_date IS NULL");
    txn.commit();

    std::cout << "Книга возвращена\n";
}

void findAuthors(pqxx::connection* conn) {
    std::string name;

    std::cout << "Имя автора для поиска: ";
    std::getline(std::cin, name);

    pqxx::work txn(*conn);
    auto result = txn.exec("SELECT * FROM authors WHERE name LIKE '%" + name + "%'");

    std::cout << "Найдено авторов: " << result.size() << "\n";
    for (auto row : result) {
        std::cout << row["id"].as<int>() << ". "
            << row["name"].c_str() << " ("
            << row["birth_year"].as<int>() << ")\n";
    }
    txn.commit();
}

void findBooks(pqxx::connection* conn) {
    std::string title;

    std::cout << "Название книги для поиска: ";
    std::getline(std::cin, title);

    pqxx::work txn(*conn);
    auto result = txn.exec("SELECT * FROM books WHERE title LIKE '%" + title + "%'");

    std::cout << "Найдено книг: " << result.size() << "\n";
    for (auto row : result) {
        std::cout << row["id"].as<int>() << ". "
            << row["title"].c_str() << " ("
            << row["year"].as<int>() << ") - "
            << row["genre"].c_str() << "\n";
    }
    txn.commit();
}

void booksByAuthor(pqxx::connection* conn) {
    int author_id;

    std::cout << "ID автора: ";
    std::cin >> author_id;
    std::cin.ignore();

    pqxx::work txn(*conn);
    auto result = txn.exec("SELECT * FROM books WHERE author_id = " +
        txn.quote(author_id));

    std::cout << "Книги автора:\n";
    for (auto row : result) {
        std::cout << row["id"].as<int>() << ". "
            << row["title"].c_str() << " ("
            << row["year"].as<int>() << ")\n";
    }
    txn.commit();
}

void top3Books(pqxx::connection* conn) {
    pqxx::work txn(*conn);
    auto result = txn.exec(
        "SELECT books.title, COUNT(*) as count "
        "FROM borrowed "
        "JOIN books ON borrowed.book_id = books.id "
        "GROUP BY books.title "
        "ORDER BY count DESC "
        "LIMIT 3"
    );

    std::cout << "Топ 3 популярные книги:\n";
    int i = 1;
    for (auto row : result) {
        std::cout << i++ << ". " << row["title"].c_str()
            << " (взята " << row["count"].as<int>() << " раз)\n";
    }
    txn.commit();
}

void recentBorrows(pqxx::connection* conn) {
    pqxx::work txn(*conn);
    auto result = txn.exec(
        "SELECT books.title, users.name, borrow_date "
        "FROM borrowed "
        "JOIN books ON borrowed.book_id = books.id "
        "JOIN users ON borrowed.user_id = users.id "
        "WHERE borrow_date >= CURRENT_DATE - INTERVAL '30 days' "
        "AND return_date IS NULL"
    );

    std::cout << "Книги взятые за 30 дней:\n";
    for (auto row : result) {
        std::cout << row["title"].c_str() << " - "
            << row["name"].c_str() << " ("
            << row["borrow_date"].c_str() << ")\n";
    }
    txn.commit();
}

void usersLastYear(pqxx::connection* conn) {
    pqxx::work txn(*conn);
    auto result = txn.exec(
        "SELECT COUNT(*) as count "
        "FROM users "
        "WHERE reg_date >= CURRENT_DATE - INTERVAL '1 year'"
    );

    int count = result[0]["count"].as<int>();
    std::cout << "Пользователей за последний год: " << count << "\n";
    txn.commit();
}

void showMenu() {
    std::cout << "\nБиблиотека\n";
    std::cout << "1. Добавить автора\n";
    std::cout << "2. Добавить книгу\n";
    std::cout << "3. Добавить пользователя\n";
    std::cout << "4. Взять книгу\n";
    std::cout << "5. Вернуть книгу\n";
    std::cout << "6. Найти авторов\n";
    std::cout << "7. Найти книги\n";
    std::cout << "8. Книги автора\n";
    std::cout << "9. Топ 3 книги\n";
    std::cout << "10. Книги за 30 дней\n";
    std::cout << "11. Пользователи за год\n";
    std::cout << "0. Выход\n";
    std::cout << "Выбор: ";
}

int main() {
    try {
        std::string conn_str = "dbname=library user=postgres password=20Dec@07$ host=localhost";
        pqxx::connection conn(conn_str);

        if (!conn.is_open()) {
            std::cout << "Ошибка подключения\n";
            return 1;
        }
        std::cout << "Подключено к БД\n";

        int choice;
        do {
            showMenu();
            std::cin >> choice;
            std::cin.ignore();

            switch (choice) {
            case 1: addAuthor(&conn); break;
            case 2: addBook(&conn); break;
            case 3: addUser(&conn); break;
            case 4: borrowBook(&conn); break;
            case 5: returnBook(&conn); break;
            case 6: findAuthors(&conn); break;
            case 7: findBooks(&conn); break;
            case 8: booksByAuthor(&conn); break;
            case 9: top3Books(&conn); break;
            case 10: recentBorrows(&conn); break;
            case 11: usersLastYear(&conn); break;
            case 0: std::cout << "Выход\n"; break;
            default: std::cout << "Неверный выбор\n";
            }

        } while (choice != 0);

    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
