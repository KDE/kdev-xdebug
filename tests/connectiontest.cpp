/*
 * XDebug Debugger Support
 *
 * Copyright 2009 Niko Sams <niko.sams@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "connectiontest.h"

#include <iostream>
#include <QtTest/QSignalSpy>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDir>

#include <KDebug>
#include <KProcess>
#include <KConfig>
#include <qtest_kde.h>

#include <shell/shellextension.h>
#include <interfaces/idebugcontroller.h>
#include <debugger/breakpoint/breakpointmodel.h>
#include <debugger/breakpoint/breakpoint.h>
#include <interfaces/ilaunchconfiguration.h>
#include <tests/testcore.h>
#include <tests/autotestshell.h>
#include <debugger/interfaces/iframestackmodel.h>
#include <debugger/variable/variablecollection.h>
#include <debugger/interfaces/ivariablecontroller.h>

#include "connection.h"
#include "debugsession.h"
#include "launchconfig.h"
#include "debugjob.h"

using namespace XDebug;
namespace KParts {
    class MainWindow;
}
namespace Sublime {
    class Controller;
}
namespace KDevelop {
    class IToolViewFactory;
}

class TestLaunchConfiguration : public KDevelop::ILaunchConfiguration
{
public:
    TestLaunchConfiguration(KUrl script) {
        c = new KConfig();
        cfg = c->group("launch");
        cfg.writeEntry("isExecutable", true);
        cfg.writeEntry("Executable", script);
        cfg.writeEntry("Interpreter", "php");
    }
    ~TestLaunchConfiguration() {
        delete c;
    }
    virtual const KConfigGroup config() const { return cfg; }
    virtual KConfigGroup config() { return cfg; }
    virtual QString name() const { return QString("Test-Launch"); }
    virtual KDevelop::IProject* project() const { return 0; }
    virtual KDevelop::LaunchConfigurationType* type() const { return 0; }
private:
    KConfigGroup cfg;
    KConfig *c;
};

#define COMPARE_DATA(index, expected) \
    compareData((index), (expected), __FILE__, __LINE__)
void compareData(QModelIndex index, QString expected, const char *file, int line)
{
    QString s = index.model()->data(index, Qt::DisplayRole).toString();
    if (s != expected) {
        kFatal() << QString("'%0' didn't match expected '%1' in %2:%3").arg(s).arg(expected).arg(file).arg(line);
    }
}


void ConnectionTest::init()
{
    qRegisterMetaType<DebugSession*>("DebugSession*");
    qRegisterMetaType<KUrl>("KUrl");

    KDevelop::AutoTestShell::init();
    m_core = new KDevelop::TestCore();
    m_core->initialize(KDevelop::Core::NoUi);

    //remove all breakpoints - so we can set our own in the test
    KDevelop::BreakpointModel* m = KDevelop::ICore::self()->debugController()->breakpointModel();
    m->removeRows(0, m->rowCount());

    /*
    KDevelop::VariableCollection *vc = KDevelop::ICore::self()->debugController()->variableCollection();
    for (int i=0; i < vc->watches()->childCount(); ++i) {
        delete vc->watches()->child(i);
    }
    vc->watches()->clear();
    */
}

void ConnectionTest::cleanup()
{
    m_core->cleanup();
    delete m_core;
}

void ConnectionTest::testStdOutput()
{
    QStringList contents;
    contents << "<?php"
            << "$i = 0;"
            << "echo \"foo\\n\";"
            << "$i++;"
            << "echo \"foo\";"
            << "echo \"bar\";"
            << "echo \"\\n\";";
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    QSignalSpy outputLineSpy(&session, SIGNAL(outputLine(QString)));
    QSignalSpy outputSpy(&session, SIGNAL(output(QString)));

    job.start();
    
    session.waitForConnected();
    session.waitForFinished();
    {
        QCOMPARE(outputSpy.count(), 4);
        QList<QVariant> arguments = outputSpy.takeFirst();
        QCOMPARE(arguments.count(), 1);
        QCOMPARE(arguments.first().toString(), QString("foo\n"));
    }
    {
        QCOMPARE(outputLineSpy.count(), 2);
        QList<QVariant> arguments = outputLineSpy.takeFirst();
        QCOMPARE(arguments.count(), 1);
        QCOMPARE(arguments.at(0).toString(), QString("foo"));
        arguments = outputLineSpy.takeFirst();
        QCOMPARE(arguments.count(), 1);
        QCOMPARE(arguments.at(0).toString(), QString("foobar"));
    }
}

void ConnectionTest::testShowStepInSource()
{
    QStringList contents;
    contents << "<?php"
            << "$i = 0;"
            << "$i++;";
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 1);

    QSignalSpy showStepInSourceSpy(&session, SIGNAL(showStepInSource(KUrl, int, QString)));

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    session.stepInto();
    session.waitForState(DebugSession::PausedState);
    session.run();
    session.waitForFinished();

    {
        QCOMPARE(showStepInSourceSpy.count(), 2);
        QList<QVariant> arguments = showStepInSourceSpy.takeFirst();
        QCOMPARE(arguments.first().value<KUrl>(), url);
        QCOMPARE(arguments.at(1).toInt(), 1);

        arguments = showStepInSourceSpy.takeFirst();
        QCOMPARE(arguments.first().value<KUrl>(), url);
        QCOMPARE(arguments.at(1).toInt(), 2);
    }
}


void ConnectionTest::testMultipleSessions()
{
    QStringList contents;
    contents << "<?php"
            << "$i = 0;"
            << "$i++;";
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    for (int i=0; i<10; ++i) {

        DebugSession session;

        TestLaunchConfiguration cfg(url);
        XDebugJob job(&session, &cfg);

        job.start();
        session.waitForConnected();
        session.waitForFinished();
    }
}

void ConnectionTest::testStackModel()
{
    QStringList contents;
    contents << "<?php"         // 1
            << "function x() {" // 2
            << "  echo 'x';"    // 3
            << "}"              // 4
            << "x();"           // 5
            << "echo 'y';";     // 6
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 1);

    job.start();
    session.waitForConnected();
    session.waitForState(DebugSession::PausedState);
    
    //step into function
    for (int i=0; i<2; ++i) {
        session.stepInto();
        session.waitForState(DebugSession::PausedState);
    }
    QTest::qWait(100);

    KDevelop::IFrameStackModel* stackModel = session.frameStackModel();

    QCOMPARE(stackModel->rowCount(QModelIndex()), 1); //one fake thread

    QModelIndex tIdx = stackModel->index(0,0);
    QCOMPARE(stackModel->rowCount(tIdx), 2);
    QCOMPARE(stackModel->columnCount(tIdx), 3);
    COMPARE_DATA(tIdx.child(0, 0), "0");
    COMPARE_DATA(tIdx.child(0, 1), "x");
    COMPARE_DATA(tIdx.child(0, 2), url.toLocalFile()+":3");
    COMPARE_DATA(tIdx.child(1, 0), "1");
    COMPARE_DATA(tIdx.child(1, 1), "{main}");
    COMPARE_DATA(tIdx.child(1, 2), url.toLocalFile()+":5");

    session.stepInto();
    session.waitForState(DebugSession::PausedState);
    session.stepInto();
    session.waitForState(DebugSession::PausedState);
    QTest::qWait(100);
    QCOMPARE(stackModel->rowCount(tIdx), 1);

    session.run();
    session.waitForFinished();
}


void ConnectionTest::testBreakpoint()
{
    QStringList contents;
    contents << "<?php"         // 1
            << "function x() {" // 2
            << "  echo 'x';"    // 3
            << "}"              // 4
            << "x();"           // 5
            << "echo 'y';";     // 6
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);


    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 3);

    job.start();
    session.waitForConnected();

    QSignalSpy showStepInSourceSpy(&session, SIGNAL(showStepInSource(KUrl, int, QString)));

    session.waitForState(DebugSession::PausedState);

    session.run();
    session.waitForFinished();
    {
        QCOMPARE(showStepInSourceSpy.count(), 1);
        QList<QVariant> arguments = showStepInSourceSpy.takeFirst();
        QCOMPARE(arguments.first().value<KUrl>(), url);
        QCOMPARE(arguments.at(1).toInt(), 3);
    }
}

void ConnectionTest::testDisableBreakpoint()
{
    QStringList contents;
    contents << "<?php"         // 1
            << "function x() {" // 2
            << "  echo 'x';"    // 3
            << "  echo 'z';"    // 4
            << "}"              // 5
            << "x();"           // 6
            << "x();"           // 7
            << "echo 'y';";     // 8
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    KDevelop::BreakpointModel* breakpoints = KDevelop::ICore::self()->debugController()->breakpointModel();
    KDevelop::Breakpoint *b;

    //add disabled breakpoint before startProgram
    b = breakpoints->addCodeBreakpoint(url, 2);
    b->setData(KDevelop::Breakpoint::EnableColumn, false);

    b = breakpoints->addCodeBreakpoint(url, 3);

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    
    //disable existing breakpoint
    b->setData(KDevelop::Breakpoint::EnableColumn, false);

    //add another disabled breakpoint
    b = breakpoints->addCodeBreakpoint(url, 7);
    QTest::qWait(300);
    b->setData(KDevelop::Breakpoint::EnableColumn, false);

    session.run();
    session.waitForFinished();
}

void ConnectionTest::testChangeLocationBreakpoint()
{
    QStringList contents;
    contents << "<?php"         // 1
            << "function x() {" // 2
            << "  echo 'x';"    // 3
            << "  echo 'z';"    // 4
            << "}"              // 5
            << "x();"           // 6
            << "x();"           // 7
            << "echo 'y';";     // 8
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    KDevelop::BreakpointModel* breakpoints = KDevelop::ICore::self()->debugController()->breakpointModel();

    KDevelop::Breakpoint *b = breakpoints->addCodeBreakpoint(url, 5);

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);

    b->setLine(7);
    QTest::qWait(100);
    session.run();

    QTest::qWait(100);
    session.waitForState(DebugSession::PausedState);

    session.run();
    session.waitForFinished();
}

void ConnectionTest::testDeleteBreakpoint()
{
    QStringList contents;
    contents << "<?php"         // 1
            << "function x() {" // 2
            << "  echo 'x';"    // 3
            << "  echo 'z';"    // 4
            << "}"              // 5
            << "x();"           // 6
            << "x();"           // 7
            << "echo 'y';";     // 8
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    KDevelop::BreakpointModel* breakpoints = KDevelop::ICore::self()->debugController()->breakpointModel();

    QCOMPARE(KDevelop::ICore::self()->debugController()->breakpointModel()->rowCount(), 1); //one for the "insert here" entry
    //add breakpoint before startProgram
    KDevelop::Breakpoint *b = breakpoints->addCodeBreakpoint(url, 5);
    QCOMPARE(KDevelop::ICore::self()->debugController()->breakpointModel()->rowCount(), 2);
    breakpoints->removeRow(0);
    QCOMPARE(KDevelop::ICore::self()->debugController()->breakpointModel()->rowCount(), 1);

    b = breakpoints->addCodeBreakpoint(url, 2);

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);

    breakpoints->removeRow(0);

    QTest::qWait(100);
    session.run();
    session.waitForFinished();
}


void ConnectionTest::testConditionalBreakpoint()
{
    QStringList contents;
    contents << "<?php"        // 1
            << "$i = 0;"       // 2
            << "$i++;"         // 3
            << "$i++;"         // 4
            << "$i++;"         // 5
            << "echo 'y';";    // 6
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);


    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 2)
        ->setCondition("1==2");
    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 5)
        ->setCondition("1==1");

    job.start();
    session.waitForConnected();

    QSignalSpy showStepInSourceSpy(&session, SIGNAL(showStepInSource(KUrl, int, QString)));

    session.waitForState(DebugSession::PausedState);

    session.run();
    session.waitForFinished();
    {
        QCOMPARE(showStepInSourceSpy.count(), 1);
        QList<QVariant> arguments = showStepInSourceSpy.takeFirst();
        QCOMPARE(arguments.first().value<KUrl>(), url);
        QCOMPARE(arguments.at(1).toInt(), 5);
    }
}


void ConnectionTest::testBreakpointError()
{
    QStringList contents;
    contents << "<?php"        // 1
            << "$i = 0;"       // 2
            << "$i++;";         // 3
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;
    KDevelop::ICore::self()->debugController()->addSession(&session);

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 2);

    KDevelop::Breakpoint* b = KDevelop::ICore::self()->debugController()->breakpointModel()
        ->addCodeBreakpoint(KUrl(""), 2);
    QVERIFY(b->errorText().isEmpty());


    job.start();
    session.waitForConnected();
    session.waitForState(DebugSession::PausedState);
    kDebug() << b->errors();
    kDebug() << b->errorText();
    QVERIFY(!b->errorText().isEmpty());

    session.run();
    session.waitForFinished();
}



KDevelop::VariableCollection *variableCollection()
{
    return KDevelop::ICore::self()->debugController()->variableCollection();
}

void ConnectionTest::testVariablesLocals()
{
    QStringList contents;
    contents << "<?php"                 // 1
            << "$foo = 'foo';"          // 2
            << "$bar = 123;"            // 3
            << "$baz = array(1, 2, 5);" // 4
            << "echo '';";              // 5
/*
<response xmlns="urn:debugger_protocol_v1" xmlns:xdebug="http://xdebug.org/dbgp/xdebug" command="context_get" transaction_id="9" context="0">
    <property name="foo" fullname="$foo" address="13397880" type="string" size="3" encoding="base64">
        <![CDATA[Zm9v]]>
    </property>
    <property name="bar" fullname="$bar" address="13397840" type="int">
        <![CDATA[123]]>
    </property>
    <property name="baz" fullname="$baz" address="13403224" type="array" children="1" numchildren="3">
        <property name="0" fullname="$baz[0]" address="13397800" type="int"><![CDATA[1]]></property>
        <property name="1" fullname="$baz[1]" address="13402872" type="int"><![CDATA[2]]></property>
        <property name="2" fullname="$baz[2]" address="13403000" type="int"><![CDATA[3]]></property>
    </property>
</response>
*/
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;
    KDevelop::ICore::self()->debugController()->addSession(&session);

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    session.variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);
    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 4);

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    QTest::qWait(1000);

    QVERIFY(variableCollection()->rowCount() >= 2);
    QModelIndex i = variableCollection()->index(1, 0);
    COMPARE_DATA(i, "Locals");
    QCOMPARE(variableCollection()->rowCount(i), 3);
    COMPARE_DATA(variableCollection()->index(2, 0, i), "$foo");
    COMPARE_DATA(variableCollection()->index(2, 1, i), "foo");
    COMPARE_DATA(variableCollection()->index(0, 0, i), "$bar");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "123");
    COMPARE_DATA(variableCollection()->index(1, 0, i), "$baz");
    COMPARE_DATA(variableCollection()->index(1, 1, i), "");
    i = variableCollection()->index(1, 0, i);
    QCOMPARE(variableCollection()->rowCount(i), 3);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "0");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "1");
    COMPARE_DATA(variableCollection()->index(2, 0, i), "2");
    COMPARE_DATA(variableCollection()->index(2, 1, i), "5");
    session.run();
    session.waitForFinished();
}


void ConnectionTest::testVariablesSuperglobals()
{
QStringList contents;
    contents << "<?php"                 // 1
            << "$foo = 'foo';";         // 2

    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;
    KDevelop::ICore::self()->debugController()->addSession(&session);

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    session.variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);
    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 1);

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    QTest::qWait(1000);

    QVERIFY(variableCollection()->rowCount() >= 3);
    QModelIndex i = variableCollection()->index(2, 0);
    COMPARE_DATA(i, "Superglobals");
    QVERIFY(variableCollection()->rowCount(i) > 4);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "$_COOKIE");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "");
    COMPARE_DATA(variableCollection()->index(1, 0, i), "$_ENV");
    COMPARE_DATA(variableCollection()->index(1, 1, i), "");
    COMPARE_DATA(variableCollection()->index(6, 0, i), "$_SERVER");
    COMPARE_DATA(variableCollection()->index(6, 1, i), "");
    i = variableCollection()->index(6, 0, i);
    QVERIFY(variableCollection()->rowCount(i) > 5);
    session.run();
    session.waitForFinished();
}
void ConnectionTest::testVariableExpanding()
{
    QStringList contents;
    contents << "<?php"                 // 1
            << "$foo = array(array(array(1, 2, 5)));" // 2
            << "echo '';";              // 3

    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;
    KDevelop::ICore::self()->debugController()->addSession(&session);

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    session.variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);
    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 2);

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    QTest::qWait(1000);

    QVERIFY(variableCollection()->rowCount() >= 2);
    QModelIndex i = variableCollection()->index(1, 0);
    COMPARE_DATA(i, "Locals");
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "$foo");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "");
    i = i.child(0, 0);
    variableCollection()->expanded(i);
    QTest::qWait(1000);
    QCOMPARE(variableCollection()->rowCount(i), 1);
    i = i.child(0, 0);
    variableCollection()->expanded(i);
    QTest::qWait(1000);
    QCOMPARE(variableCollection()->rowCount(i), 1);
    i = i.child(0, 0);
    variableCollection()->expanded(i);
    QTest::qWait(1000);
    QCOMPARE(variableCollection()->rowCount(i), 3);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "0");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "1");
    COMPARE_DATA(variableCollection()->index(2, 0, i), "2");
    COMPARE_DATA(variableCollection()->index(2, 1, i), "5");
    session.run();
    session.waitForFinished();
}

void ConnectionTest::testPhpCrash()
{
    QStringList contents;
    contents << "<?php"
            << "$i = 0;"
            << "$i++;";
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob *job = new XDebugJob(&session, &cfg);

    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 1);

    job->start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    job->process()->kill(); //simulate crash
    QTest::qWait(1000);
    QCOMPARE(session.state(), DebugSession::NotStartedState); //well, it should be EndedState in reality, but this works too

    //job seems to gets deleted automatically
}

void ConnectionTest::testConnectionClosed()
{
    QStringList contents;
    contents << "<?php"
            << "$i = 0;"
            << "$i++;";
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;

    TestLaunchConfiguration cfg(url);
    XDebugJob *job = new XDebugJob(&session, &cfg);

    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 1);

    job->start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    session.connection()->close(); //simulate eg webserver restart
    QTest::qWait(1000);
    QCOMPARE(session.state(), DebugSession::NotStartedState); //well, it should be EndedState in reality, but this works too

    //job seems to gets deleted automatically
}


//see bug 235061
void ConnectionTest::testMultipleConnectionsClosed()
{
    QStringList contents;
    contents << "<?php echo 123;"
            << "$i = 0;"
            << "$i++;";
    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;
    session.setAcceptMultipleConnections(true);

    TestLaunchConfiguration cfg(url);
    XDebugJob *job = new XDebugJob(&session, &cfg);

    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 1);

    job->start();
    session.waitForConnected();
    Connection* firstConnection = session.connection();

    session.waitForState(DebugSession::PausedState);

    //start a second process, as XDebugBrowserJob does
    KProcess secondProcess;
    secondProcess.setProgram(job->process()->program());
    secondProcess.setEnvironment(job->process()->environment());
    secondProcess.setOutputChannelMode(KProcess::ForwardedChannels);
    secondProcess.start();

    QTest::qWait(1000);
    QVERIFY(session.connection() != firstConnection); //must be a different connection

    session.connection()->close(); //close second connection
    QTest::qWait(1000);

//     firstConnection->close(); //close first connection _after_ second

    QTest::qWait(1000);
    QCOMPARE(session.state(), DebugSession::NotStartedState); //well, it should be EndedState in reality, but this works too

    //job seems to gets deleted automatically
}

void ConnectionTest::testVariableUpdates()
{
    QStringList contents;
    contents << "<?php"                 // 1
            << "$foo = 1;"          // 2
            << "$foo++;"            // 3
            << "$foo++;";            // 4

    QTemporaryFile file("xdebugtest");
    file.open();
    KUrl url(file.fileName());
    file.write(contents.join("\n").toUtf8());
    file.close();

    DebugSession session;
    KDevelop::ICore::self()->debugController()->addSession(&session);

    TestLaunchConfiguration cfg(url);
    XDebugJob job(&session, &cfg);

    session.variableController()->setAutoUpdate(KDevelop::IVariableController::UpdateLocals);
    KDevelop::ICore::self()->debugController()->breakpointModel()->addCodeBreakpoint(url, 2);

    job.start();
    session.waitForConnected();

    session.waitForState(DebugSession::PausedState);
    QTest::qWait(1000);

    QVERIFY(variableCollection()->rowCount() >= 2);
    QModelIndex i = variableCollection()->index(1, 0);
    COMPARE_DATA(i, "Locals");
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "$foo");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "1");

    session.stepInto();
    QTest::qWait(1000);
    QVERIFY(variableCollection()->rowCount() >= 2);
    i = variableCollection()->index(1, 0);
    COMPARE_DATA(i, "Locals");
    QCOMPARE(variableCollection()->rowCount(i), 1);
    COMPARE_DATA(variableCollection()->index(0, 0, i), "$foo");
    COMPARE_DATA(variableCollection()->index(0, 1, i), "2");

    session.run();
    session.waitForFinished();
}

//     controller.connection()->sendCommand("eval -i 124", QStringList(), "eval(\"function test124() { return rand(); } return test124();\")");
//     controller.connection()->sendCommand("eval -i 126", QStringList(), "test124();");


QTEST_KDEMAIN(ConnectionTest, GUI)

#include "connectiontest.moc"
