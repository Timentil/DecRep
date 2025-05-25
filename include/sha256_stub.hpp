std::string sha256(const std::string &str)
{
    std::string new_str = "<hashed>" + str + "<hashed>";
    return new_str;
}