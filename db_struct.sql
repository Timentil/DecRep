CREATE TABLE IF NOT EXISTS Users(
    id SERIAL PRIMARY KEY,
    username VARCHAR(255) NOT NULL UNIQUE,
    first_connection_time TIMESTAMP NOT NULL
);

CREATE TABLE IF NOT EXISTS Files(
    id SERIAL PRIMARY KEY,
    file_name VARCHAR(255) NOT NULL,
    file_size BIGINT NOT NULL,
    addition_time TIMESTAMP NOT NULL,
    last_modified TIMESTAMP NOT NULL,
    DecRep_path VARCHAR(255) NOT NULL,
    author_id INTEGER NOT NULL REFERENCES Users(id)
);

CREATE TABLE IF NOT EXISTS FileOwners(
    owner_id INTEGER REFERENCES Users(id),
    file_id INTEGER REFERENCES Files(id),
    local_path VARCHAR(255) NOT NULL,
    PRIMARY KEY(owner_id, file_id)
);

CREATE TABLE IF NOT EXISTS MyUsername (
    username VARCHAR(255) PRIMARY KEY
);

