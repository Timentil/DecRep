CREATE TABLE IF NOT EXISTS Users (
    id SERIAL PRIMARY KEY,
    ip INET,
    username VARCHAR(255),
    first_connection_time TIMESTAMP );


CREATE TABLE IF NOT EXISTS Files (
               id SERIAL PRIMARY KEY,
               file_name VARCHAR(255),
               file_size BIGINT,             --(исходный)
               file_hash VARCHAR(255),       --type
               addition_time TIMESTAMP,      --CreationTime
               last_modified TIMESTAMP,      --???
               DecRep_path VARCHAR(255),
               author_id INTEGER REFERENCES Users(id) );


CREATE TABLE IF NOT EXISTS FileOwners (
               owner_id INTEGER REFERENCES Users(id),
               file_id INTEGER REFERENCES Files(id),
               local_path VARCHAR(255),
               PRIMARY KEY (owner_id, file_id) );