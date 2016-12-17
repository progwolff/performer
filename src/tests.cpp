#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "util.h"


TEST_CASE( "try_run cancels after timeout", "[util]" ) 
{
    QTime timer;
    timer.start();
    try_run(200, [](){QThread::msleep(300);});
    REQUIRE( timer.elapsed() < 250 );
}

TEST_CASE( "try_run does not increase execution time of a function returning before the timeout elapsed", "[util]" ) 
{
    QTime timer;
    timer.start();
    try_run(200, [](){QThread::msleep(100);});
    REQUIRE( timer.elapsed() < 150 );
}

TEST_CASE( "replace occurences of a in str with b with input data type: const char*", "[util]" )
{
    const char* str = "string";
    const char* a = "ring";
    const char* b = "rong";
    
    REQUIRE( QString::fromLatin1(replace(str, a, b)) == "strong" );
}


TEST_CASE( "replace occurences of a in str with b with input data type: QString&", "[util]" )
{
    const char* str = "string";
    QString a = "ring";
    QString b = "rong";
    
    REQUIRE( QString::fromLatin1(replace(str, a, b)) == "strong" );
}

#include "setlistmetadata.h"

TEST_CASE ( "Constructor, getters and update of SetlistMetadata are working", "[setlistmetadata]" )
{
    QVariantMap map;
    map.insert("patch", QUrl::fromLocalFile("testpatch"));
    map.insert("notes", QUrl::fromLocalFile("testnotes"));
    map.insert("preload",false);
    map.insert("name","testname");
    SetlistMetadata testmetadata("test", map);
    REQUIRE( testmetadata.name() == "testname" );
    REQUIRE( testmetadata.patch().toLocalFile() == "testpatch" );
    REQUIRE( testmetadata.notes().toLocalFile() == "testnotes" );
    REQUIRE( testmetadata.preload() == false );
    
    map.insert("patch", QUrl::fromLocalFile("testpatch2"));
    map.insert("notes", QUrl::fromLocalFile("testnotes2"));
    map.insert("preload", true);
    map.insert("name", "testname2");
    testmetadata.update(map);
    REQUIRE( testmetadata.name() == "testname2" );
    REQUIRE( testmetadata.patch().toLocalFile() == "testpatch2" );
    REQUIRE( testmetadata.notes().toLocalFile() == "testnotes2" );
    REQUIRE( testmetadata.preload() == true );
    
}

#include "midi.h"
#include <QToolButton>
#include <QApplication>

TEST_CASE ( "MIDI actions can be added and returned" )
{
    QAction action("Test");
    action.setObjectName("TestAction");
    QAction action2("Test2");
    action2.setObjectName("TestAction2");
    MIDI::addAction(&action);
    MIDI::addAction(&action2);
    
    REQUIRE( MIDI::actions().size() == 2 );
    REQUIRE( MIDI::actions()[0]->objectName() == "TestAction" );
    REQUIRE( MIDI::actions()[1]->objectName() == "TestAction2" );
}

TEST_CASE ( "MIDI CCs can be assigned to actions" )
{
    QAction action("Test");
    action.setObjectName("TestAction");
    QAction action2("Test2");
    action2.setObjectName("TestAction2");
    MIDI::addAction(&action);
    MIDI::addAction(&action2);
    
    MIDI::setCc(&action, 4);
    MIDI::setCc(&action2, 64);
    
    REQUIRE( MIDI::cc(&action) == 4 );
    REQUIRE( MIDI::cc(&action2) == 64 );
}

TEST_CASE ( "MIDI learn for QWidgets" )
{
    int argc = 0;
    QApplication app(argc, nullptr);
    QToolButton button;
    
    QAction *action = MIDI::setLearnable(&button, "Test", "Test");
    
    REQUIRE( button.actions()[0] == action );
    
    delete action;
}
