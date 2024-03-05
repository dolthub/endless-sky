//
// Created by Brian Hendriks on 2/26/24.
//

#ifndef ENDLESS_SKY_DB_H
#define ENDLESS_SKY_DB_H

#include <vector>
#include <string>
#include <map>

#include "/usr/local/mysql-connector-c++/include/mysql/jdbc.h"

class Rows {
public:
    virtual ~Rows() = default;

    virtual bool Next() = 0;

    virtual bool Int(std::string colName, int* out) = 0;
    virtual bool Double(std::string colName, double* out) = 0;
    virtual bool String(std::string colName, std::string* out) = 0;
    virtual bool Bool(std::string colName, bool* out) = 0;
};

class DoltRows : public Rows {
public:
    explicit DoltRows(sql::ResultSet* resultSet) : resultSet(resultSet) {}
    ~DoltRows() override;

    bool Next() override;
    bool Int(std::string colName, int* out) override;
    bool Double(std::string colName, double* out) override;
    bool String(std::string colName, std::string* out) override;
    bool Bool(std::string colName, bool* out) override;

private:
    sql::ResultSet* resultSet;
};

class InMemRows : public Rows {
public:
    explicit InMemRows(std::vector<std::map<std::string, std::string>> rows) : rows(std::move(rows)), nextIdx(-1) {}

    bool Next() override {
        this->nextIdx++;
        return this->nextIdx <= rows.size();
    }

    bool Int(std::string colName, int* out) override;
    bool Double(std::string colName, double* out) override;
    bool String(std::string colName, std::string* out) override;
    bool Bool(std::string colName, bool* out) override;

private:
    std::vector<std::map<std::string,std::string>> rows;
    int nextIdx;
};

class DB {
public:
    virtual ~DB() = default;

    virtual Rows* SelectQuery(const std::string query) = 0;
};

class DoltDB : public DB {
public:
    DoltDB();
    ~DoltDB() override;

    Rows* SelectQuery(const std::string query) override;

private:
    sql::mysql::MySQL_Driver *driver;
    sql::Connection *conn;
};

class InMemDB : public DB {
public:
    Rows* SelectQuery(const std::string query) override;
    void SetReturnRows(Rows* rows);

private:
    Rows* toReturn;
};

#endif //ENDLESS_SKY_DB_H
