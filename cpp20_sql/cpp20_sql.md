
# A thin, typesafe SQL wrapper in C++20

For data accessing databases, SQL is the champ. It is cross-platform and cross database. There is a ton of information online, and you can probably find an SQL snippet to do almost anything.

In addition, SQL has a nice  property, in that you can typically develop and debug it an interactive manner by using the database environment. You can quickly run a query, and check that you are getting the expected result. The REPL(read, eavaluate, print loop) makes for quick development.

Now that we have a working SQL query. How do we get it into C++. There are a whole host of existing solutions. However, they often fall short in a few areas:

* Typesafe SQL columns and parameters. We would like compile time typesafety when we are reading the results and when we bind parameters to our parameterized queries.

* Be able to copy and paste working SQL without too much modification and with no external code generators.

* No macros
    
C++20 provides us with improved compile time features that can allow us to accomplish these goals.

The code can be found here: [https://github.com/google/cpp-from-the-sky-down/tree/master/cpp20_sql]

`tagged_sqlite.h` is a single self-contained header with no depedencies other than the C++ standard library and SQLite.

`example.cpp` is an example program.

Note: This is *not* an officially supported Google library.

Also, it is only at a proof of concept state, and *not* ready for use in production.

Currently, only GCC 10 supports enough of C++20 to be able to use the library

The library lives in the `skydown` namespace, with the user defined literals in `skydown::literals`.

To illustrate, let us do a quick `customers` and `orders` database.

Assume we have the following tables from SQL:


``` sql
CREATE TABLE customers(
      id INTEGER NOT NULL PRIMARY KEY, 
      name TEXT NOT NULL"
      );
```

``` sql
CREATE TABLE orders(
    id INTEGER NOT NULL PRIMARY KEY,
    item TEXT NOT NULL, 
    customerid INTEGER NOT NULL,
    price REAL NOT NULL, 
    discount_code TEXT 
    );
```

Now we want to find all the orders that are above a certain price and join them with the customer's name.

```sql
SELECT orders.id, name, item, price, 
      discount_code
      FROM orders JOIN customers ON customers.id = customerid 
      WHERE price > 100;

```

This will give us a list of of orders.

Now in a program, we want to be able to specify our minimum price at runtime, instead of hard coding.

The temptation would be to use string substitution. However, that is the wrong answer, as it opens you up to all sorts of SQL injection attacks. Instead you want to do parameterized queries.

```sql
SELECT orders.id, name, item, price, 
      discount_code
      FROM orders JOIN customers ON customers.id = customerid 
      WHERE price > ?;
```

This is the syntax for a parameterized query that we use with a prepared statement. Then we just bind the value we want to our parameter and execute the statement.

Now let us see what the code looks like to do this query and get the results.

```c++
skydown::prepared_statement<
      "SELECT orders.id:integer, name:text, item:text, price:real, "
      "discount_code:text? "
      "FROM orders JOIN customers ON customers.id = customerid "
      "WHERE price > ?min_price:real;"
      >select_orders{sqldb};
```

Here we are construction an object of template class `skydown::prepared_statement` passing our SQL query as the template parameter.

However, if we look closely at the query string, we will notice it is slightly different from the SQL string. 

Instead of `orders.id,` in the SQL string, we have `orders.id:integer`. Also, instead of `?` for the parameters, we have `?min_price:real`

What is going on here?

Turns out, if we can just annotate the resulting columns with types, and the parameters with names and types, we can have nicely named and typed input and output for the query, and treat the rest of the query as a black box.

The library uses those annotations to construct tags with types for parameters and fields.

Prior to sending the query to the SQL engine, it strips out the annotations. 

This allows us to not have to care about the internals of what the SQL statement is doing. We just care about the inputs (which are the annotated parameters) and the outputs(which are the annotated selected columns).

Here is the list of currently supported types:

* `:text` ==> `std::string_view`
* `:integer` ==> `std::int64_t`
* `:real` ==> `double`

You can add a `?` to the end of the type to make it `std::optional`
For example `:real?` would map to `std::optional<double>`.

Let us see how we use this in our C++ code.

```c++
for (auto &row :
        select_orders.execute_rows("min_price"_param = 100)) {
    std::cout << row["orders.id"_col] << " ";
    std::cout << row["price"_col] << " ";
    std::cout << row["name"_col] << " ";
    std::cout << row["item"_col] << " ";
    std::cout << row["discount_code"_col].value_or("<NO CODE>") << "\n";
}

```
Here we are calling the `execute_rows` member function of our `select orders` object, passing in the parameters. `_param` is a user defined string literal in namespace `skydown::literals`. If a parameter is not specified, or an incorrect string is used, or if an incorrect type is used, you will get a compile time error.


In our query for `select_orders` we had `?min_price:real` so if we assign a value `"min_price"_param` that cannot be converted to a `double` we will get a compile time error.

We use a `range for` loop to iterate ther eturned rows. `_col` is another used defined string literal, and we index the `row` object using the string literal. Again, if we have the wrong name, it is a compile time error. In addition, the returned values have the correct type according to our annotated SQL statement.

* `orders.id:integer` ==> `std::int64_t`
* `price:real` ==> `double`
* `name:text` ==> `std::string_view`
* `item:text` ==> `std::string_view`
* `discount_code:text?` ==> `std::optional<std::string_view>`

What happens, if we mess up and type the wrong column name.

To see what happens, I changed `discount_code` to `discount_core` in the following expression:

```c++
    std::cout << row["discount_core"_col].value_or("<NO CODE>") << "\n";
```

I get a compiler error which includes the following output.

```
1>C:\Users\johnb\source\repos\cpp-from-the-sky-down\cpp20_sql\tagged_sqlite.h(220,16): message : template argument deduction/substitution failed:
1>C:\Users\johnb\source\repos\cpp-from-the-sky-down\cpp20_sql\tagged_sqlite.h(228,20): message : 'skydown::sqlite_experimental::fixed_string<13>{"discount_core"}' is not equivalent to 'skydown::sqlite_experimental::fixed_string<9>{"orders.id"}'
1>C:\Users\johnb\source\repos\cpp-from-the-sky-down\cpp20_sql\tagged_sqlite.h(228,20): message : 'skydown::sqlite_experimental::fixed_string<13>{"discount_core"}' is not equivalent to 'skydown::sqlite_experimental::fixed_string<4>{"name"}'
1>C:\Users\johnb\source\repos\cpp-from-the-sky-down\cpp20_sql\tagged_sqlite.h(228,20): message : 'skydown::sqlite_experimental::fixed_string<13>{"discount_core"}' is not equivalent to 'skydown::sqlite_experimental::fixed_string<4>{"item"}'
1>C:\Users\johnb\source\repos\cpp-from-the-sky-down\cpp20_sql\tagged_sqlite.h(228,20): message : 'skydown::sqlite_experimental::fixed_string<13>{"discount_core"}' is not equivalent to 'skydown::sqlite_experimental::fixed_string<5>{"price"}'
1>C:\Users\johnb\source\repos\cpp-from-the-sky-down\cpp20_sql\tagged_sqlite.h(228,20): message : 'skydown::sqlite_experimental::fixed_string<13>{"discount_core"}' is not equivalent to 'skydown::sqlite_experimental::fixed_string<13>{"discount_code"}'


```

Notice how it is is showing us the typo, `discount_core` and showing us the actual columns: `orders.id`, `name`, `item`, `price`, `discount_code`.


Next time, we will talk a bit more about using the library, and start looking at some of the techniques used in implementing it.

If you want to play with this code, you can just compile `example.cpp` with `g++10` and link to `sqlite3`.

One of the nice things about this kind of approach, is that the library can be agnostic to the actual SQL. This allows it to be relatively small (around 600 lines of code, about half of which is interfacing to SQLite3) as well as be able to support all the features of a database, because we are just passing in SQL to be executed by the engine.

In addition, because the SQL query is a compile time string, you cannot get runtime SQL injection attacks.

Right now, only SQLite is supported, but I plan on separating out SQLite from the library and supporting multiple databases such as MySQL and Postgres SQL.

Please feel let me know what you think in the comments.




