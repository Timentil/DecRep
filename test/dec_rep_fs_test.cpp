#include "dec_rep_fs.hpp"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace DecRepFS;

class FSManagerTest : public ::testing::Test {
protected:
    FS fs_manager;
    std::vector<fs::path> temp_dirs;
    std::vector<fs::path> temp_files;

    void create_temp_file(const fs::path &path, const std::string &content)
    {
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
        temp_files.push_back(path);
    }

    void create_temp_folder(const fs::path &path)
    {
        fs::create_directories(path);
        temp_dirs.push_back(path);
    }

    std::string capture_print_output()
    {
        std::ostringstream oss;
        auto *old_buf = std::cout.rdbuf();
        std::cout.rdbuf(oss.rdbuf());
        fs_manager.print_DecRepFS();
        std::cout.rdbuf(old_buf);
        return oss.str();
    }

    void TearDown() override
    {
        for (auto &f : temp_files) {
            if (fs::exists(f)) {
                fs::remove(f);
            }
        }
        for (auto &d : temp_dirs) {
            if (fs::exists(d)) {
                fs::remove_all(d);
            }
        }
    }
};

TEST_F(FSManagerTest, AddAndFindFile)
{
    fs_manager.add_file("/foo/bar", "test.txt");
    auto paths = fs_manager.find_path("test.txt");
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0], "DecRep/foo/bar/test.txt");
}

TEST_F(FSManagerTest, AddSameFile)
{
    fs_manager.add_file("/dup", "same.txt");
    fs_manager.add_file("/dup", "same.txt");
    auto paths = fs_manager.find_path("same.txt");
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0], "DecRep/dup/same.txt");
}

TEST_F(FSManagerTest, AddFolder)
{
    create_temp_folder("temp_folder/subdir");
    create_temp_file("temp_folder/file1.txt", "c1");
    create_temp_file("temp_folder/subdir/file2.txt", "c2");
    fs_manager.add_folder("/docs", "temp_folder");
    auto p1 = fs_manager.find_path("file1.txt");
    auto p2 = fs_manager.find_path("file2.txt");
    ASSERT_EQ(p1.size(), 1);
    EXPECT_EQ(p1[0], "DecRep/docs/file1.txt");
    ASSERT_EQ(p2.size(), 1);
    EXPECT_EQ(p2[0], "DecRep/docs/subdir/file2.txt");
}

TEST_F(FSManagerTest, AddNonexistentLocalFolder)
{
    fs::path bad = "no_such_folder";
    if (fs::exists(bad)) {
        fs::remove_all(bad);
    }
    EXPECT_NO_THROW(fs_manager.add_folder("/noexist", bad.string()));
    auto paths = fs_manager.find_path("noexist");
    EXPECT_TRUE(paths.empty());
}

TEST_F(FSManagerTest, DeleteFile)
{
    fs_manager.add_file("/a", "x.txt");
    EXPECT_NO_THROW(fs_manager.delete_file("/a/x.txt"));
    EXPECT_TRUE(fs_manager.find_path("x.txt").empty());
}

TEST_F(FSManagerTest, DeleteFolderAsFile)
{
    fs_manager.add_file("/foo", "f.txt");
    EXPECT_THROW(fs_manager.delete_folder("/foo/f.txt"), std::runtime_error);
}

TEST_F(FSManagerTest, DeleteFolder)
{
    fs_manager.add_file("/d/e", "y.txt");
    EXPECT_NO_THROW(fs_manager.delete_folder("/d/e"));
    EXPECT_TRUE(fs_manager.find_path("y.txt").empty());
}

TEST_F(FSManagerTest, DeleteUserFiles)
{
    fs_manager.add_file("/batch", "f1.txt");
    fs_manager.add_file("/batch", "f2.txt");
    fs_manager.delete_user_files({ "/batch/f1.txt", "/batch/f2.txt" });
    EXPECT_TRUE(fs_manager.find_path("f1.txt").empty());
    EXPECT_TRUE(fs_manager.find_path("f2.txt").empty());
}

TEST_F(FSManagerTest, RenameFile)
{
    fs_manager.add_file("/r/path", "old.txt");
    fs_manager.rename_file("/r/path", "old.txt", "new.txt");
    EXPECT_TRUE(fs_manager.find_path("old.txt").empty());
    auto newp = fs_manager.find_path("new.txt");
    ASSERT_EQ(newp.size(), 1);
    EXPECT_EQ(newp[0], "DecRep/r/path/new.txt");
}

TEST_F(FSManagerTest, RenameFolderToExisting)
{
    fs_manager.add_file("/a", "one.txt");
    fs_manager.add_file("/b", "two.txt");
    EXPECT_THROW(fs_manager.rename_folder("/a", "/b"), std::runtime_error);
}

TEST_F(FSManagerTest, RenameFolder)
{
    fs_manager.add_file("/foo/bar", "a.txt");
    fs_manager.rename_folder("/foo", "/baz");
    auto p = fs_manager.find_path("a.txt");
    ASSERT_EQ(p.size(), 1);
    EXPECT_EQ(p[0], "DecRep/baz/bar/a.txt");
}

TEST_F(FSManagerTest, ChangePath)
{
    fs_manager.add_file("/oldp", "m.txt");
    fs_manager.change_path("m.txt", "/oldp", "/newp");
    auto paths = fs_manager.find_path("m.txt");
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0], "DecRep/newp/m.txt");
}

TEST_F(FSManagerTest, FindSevPaths)
{
    fs_manager.add_file("/dup", "same.txt");
    fs_manager.add_file("/dup/sub", "same.txt");
    EXPECT_EQ(fs_manager.find_path("same.txt").size(), 2);
}

TEST_F(FSManagerTest, FindRoot)
{
    auto roots = fs_manager.find_path("DecRep");
    ASSERT_EQ(roots.size(), 1);
    EXPECT_EQ(roots[0], "DecRep");
}

TEST_F(FSManagerTest, DeleteNonexistentFile)
{
    EXPECT_THROW(fs_manager.delete_file("/no/such.txt"), std::runtime_error);
}

TEST_F(FSManagerTest, DeleteNonexistentFolder)
{
    EXPECT_THROW(fs_manager.delete_folder("/no/folder"), std::runtime_error);
}

TEST_F(FSManagerTest, RenameFileNonexistent)
{
    EXPECT_THROW(fs_manager.rename_file("/a", "nofile.txt", "new.txt"), std::runtime_error);
}

TEST_F(FSManagerTest, ChangePathCollision)
{
    fs_manager.add_file("/a", "f.txt");
    fs_manager.add_file("/b", "f.txt");
    EXPECT_THROW(fs_manager.change_path("f.txt", "/a", "/b"), std::runtime_error);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
