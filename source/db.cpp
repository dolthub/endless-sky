//
// Created by Brian Hendriks on 2/26/24.
//

#include "db.h"
#include "logger.h"

using namespace std;
using namespace sql;
using namespace sql::mysql;



DoltDB::DoltDB()
{
    try {
        driver = get_mysql_driver_instance();
        conn = driver->connect("tcp://127.0.0.1:3306", "dolt", "");
        conn->setSchema("datadb");
    } catch (SQLException &e) {
        Logger::LogError("Failed to connect to database: " + string(e.what()) + " (error code: " + to_string(e.getErrorCode()) + ")");
    }
}

DoltDB::~DoltDB()
{
    if (conn != nullptr) {
        conn->close();
        delete conn;
    }
}

Rows* DoltDB::SelectQuery(const string query)
{
    if (conn == nullptr) {
        return nullptr;
    }

    Statement* stmt = conn->createStatement();
    ResultSet* res = stmt->executeQuery(query);
    delete stmt;

    return new DoltRows(res);
}

DoltRows::~DoltRows()
{
    delete resultSet;
    resultSet = nullptr;
}

bool DoltRows::Next()
{
    return resultSet->next();
}

bool DoltRows::Int(std::string colName, int* out)
{
    if (resultSet->isNull(colName))
        return false;

    *out = resultSet->getInt(colName);
    return true;
}

bool DoltRows::Double(std::string colName, double* out)
{
    if (resultSet->isNull(colName))
        return false;

    *out = double(resultSet->getDouble(colName));
    return true;
}

bool DoltRows::String(std::string colName, std::string* out)
{
    if (resultSet->isNull(colName))
        return false;

    *out = resultSet->getString(colName);
    return true;
}

bool DoltRows::Bool(std::string colName, bool* out)
{
    if (resultSet->isNull(colName))
        return false;

    *out = resultSet->getBoolean(colName);
    return true;
}

Rows* InMemDB::SelectQuery(const string query) {
    return toReturn;
}

void InMemDB::SetReturnRows(Rows* rows) {
    toReturn = rows;
}

bool InMemRows::Int(string colName, int* out)
{
    map<string,string> row = this->rows[this->nextIdx-1];
    if (row.find(colName) == row.end())
        return false;

    string val = row[colName];
    *out = stoi(val);
    return true;
}

bool InMemRows::Double(string colName, double* out)
{
    map<string,string> row = this->rows[this->nextIdx-1];
    if (row.find(colName) == row.end())
        return false;

    string val = row[colName];
    *out = stod(val);
    return true;
}

bool InMemRows::String(string colName, string* out)
{
    map<string,string> row = this->rows[this->nextIdx-1];
    if (row.find(colName) == row.end())
        return false;

    string val = row[colName];
    *out = val;
    return true;
}

bool InMemRows::Bool(string colName, bool* out)
{
    map<string,string> row = this->rows[this->nextIdx-1];
    if (row.find(colName) == row.end())
        return false;

    string val = row[colName];

    // lowercase str
    string lower;
    for (char c : val)
        lower += char(tolower(c));

    *out = (lower != "false" && lower != "0");
    return true;
}
