// The MIT License( MIT )
//
// Copyright( c ) 2020 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "MainWindow.h"
#include "SABUtils/AutoWaitCursor.h"
#include "SABUtils/MD5.h"

#include "ui_MainWindow.h"

#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>
#include <QCompleter>
#include <QTimer>
#include <QDebug>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QProgressDialog>
#include <QHeaderView>


CMainWindow::CMainWindow(QWidget* parent)
    : QMainWindow(parent),
    fImpl(new Ui::CMainWindow)
{
    fImpl->setupUi(this);
    fLHS.setTable( fImpl->lhsData );
    fLHS.setTotalCount( fImpl->numLHSRows );
    fLHS.setSubCount( fImpl->numLHSOnly );
    fLHS.setMergedColumns( fImpl->mergedColumnsLHS );
    fLHS.setExtraColumns( fImpl->extraColumnsLHS );
    fLHS.setMatchedColumns( fImpl->matchedColumnsLHS );
    fLHS.setIgnoredRows( fImpl->ignoredRowsLHS );

    fRHS.setTable( fImpl->rhsData );
    fRHS.setTotalCount( fImpl->numRHSRows );
    fRHS.setSubCount( fImpl->numRHSOnly );
    fRHS.setMergedColumns( fImpl->mergedColumnsRHS );
    fRHS.setExtraColumns( fImpl->extraColumnsRHS );
    fRHS.setMatchedColumns( fImpl->matchedColumnsRHS );
    fRHS.setIgnoredRows( fImpl->ignoredRowsRHS );

    fMerged.setTable( fImpl->mergeData );
    fMerged.setTotalCount( fImpl->numTotalRows );
    fMerged.setSubCount( fImpl->numMatchedRows );

    loadSettings();

    connect( fImpl->resultsTree, &QTreeWidget::currentItemChanged, this, &CMainWindow::slotResultsItemChanged );
    connect( fImpl->compareBtn, &QPushButton::clicked, this, &CMainWindow::slotLoad );
    connect(fImpl->lhsFile, &QLineEdit::textChanged, this, &CMainWindow::slotFilesChanged);
    connect(fImpl->rhsFile, &QLineEdit::textChanged, this, &CMainWindow::slotFilesChanged);
    connect(fImpl->btnSelectLHSFile, &QToolButton::clicked, this, &CMainWindow::slotSelectLHSFile);
    connect(fImpl->btnSelectRHSFile, &QToolButton::clicked, this, &CMainWindow::slotSelectRHSFile);
    connect(fImpl->saveBtn, &QPushButton::clicked, this, &CMainWindow::slotSave);

    auto completer = new QCompleter(this);
    auto fsModel = new QFileSystemModel(completer);
    fsModel->setRootPath("");
    completer->setModel(fsModel);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    fImpl->lhsFile->setCompleter(completer);
    fImpl->rhsFile->setCompleter(completer);

    QTimer::singleShot(0, this, &CMainWindow::slotFilesChanged);
    slotResultsItemChanged( nullptr, nullptr );
}

CMainWindow::~CMainWindow()
{
    saveSettings();
}

void CMainWindow::loadSettings()
{
    QSettings settings;

    fImpl->lhsFile->setText(settings.value("LHSFile", QString()).toString());
    fImpl->rhsFile->setText(settings.value("RHSFile", QString()).toString());
}

void CMainWindow::saveSettings()
{
    QSettings settings;

    settings.setValue("LHSFile", fImpl->lhsFile->text());
    settings.setValue("RHSFile", fImpl->rhsFile->text());
}

void CMainWindow::slotFilesChanged()
{
    QFileInfo lhs(fImpl->lhsFile->text());
    bool aOK = !fImpl->lhsFile->text().isEmpty() && lhs.exists() && lhs.isFile();

    QFileInfo rhs(fImpl->rhsFile->text());
    aOK = aOK && !fImpl->rhsFile->text().isEmpty() && rhs.exists() && rhs.isFile();

    fImpl->compareBtn->setEnabled( aOK );
    if ( aOK )
        QTimer::singleShot( 0, [this]()
                            {
                                fImpl->compareBtn->animateClick();
                            }
    );
}

void CMainWindow::slotSelectLHSFile()
{
    auto file = QFileDialog::getOpenFileName( this, tr("Select LHS File:"), fImpl->lhsFile->text(), tr( "CSV File (*.csv);;Text Files(*.txt);;All Files(*.*)" ) );
    if (!file.isEmpty())
        fImpl->lhsFile->setText( file );
}

void CMainWindow::slotSelectRHSFile()
{
    auto file = QFileDialog::getOpenFileName( this, tr( "Select RHS File:" ), fImpl->rhsFile->text(), tr( "CSV File (*.csv);;Text Files(*.txt);;All Files(*.*)" ) );
    if (!file.isEmpty())
        fImpl->rhsFile->setText( file );
}

void CMainWindow::slotLoad()
{
    loadFiles();
}

void CMainWindow::loadFiles()
{
    NSABUtils::CAutoWaitCursor awc;

    clear();
    if ( !fLHS.loadFile( fImpl->lhsFile->text(), this ) )
    {
        clear();
        return;
    }

    if ( !fRHS.loadFile( fImpl->rhsFile->text(), this ) )
    {
        clear();
        return;
    }

    if ( !SFileData::mergeData( fLHS, fRHS, fMerged, this ) )
    {
        clear();
        return;
    }
    fImpl->mergeData->sortByColumn( 0, Qt::SortOrder::AscendingOrder );

    fImpl->numMatchedColumns->setText( QString::number( fLHS.numImportantColumns() ) );
    fLHS.updateMatchedColumns();
    fRHS.updateMatchedColumns();
}

void SFileData::clear()
{
    if ( fTable.first )
    {
        fTable.first->clear();
        fTable.first->setRowCount( 0 );
        fTable.first->setColumnCount( 0 );
    }
    if ( fTable.second.second )
        fTable.second.second->clear();

    if ( fSubCount )
        fSubCount->setText( QString() );
    if ( fTotalCount )
        fTotalCount->clear();
    if ( fMergedColumns )
        fMergedColumns->clear();
    if ( fExtraColumns )
        fExtraColumns->clear();
    if ( fMatchedColumns )
        fMatchedColumns->clear();
    if ( fIgnoredRows )
        fIgnoredRows->clear();

    fRowToMD5.clear();
    fMD5ToRow.clear();
    fHeaderInfo.clear();
    fImportantCols.clear();
}

void CMainWindow::clear()
{
    fLHS.clear();
    fRHS.clear();
    fMerged.clear();

    fImpl->numMatchedColumns->setText( QString() );
}

void CMainWindow::slotSave()
{
    fMerged.save( this );
}

void SFileData::save( QWidget * parent )
{
    auto fn = QFileDialog::getSaveFileName( parent, QObject::tr( "Merged File:" ), QString(), QObject::tr( "CSV File (*.csv);;Text Files(*.txt);;All Files(*.*)" ) );
    if ( fn.isEmpty() )
        return;

    QFile file( fn );
    file.open( QFile::Text | QFile::Truncate | QFile::WriteOnly );
    if ( !file.isOpen() )
    {
        QMessageBox::critical( parent, QObject::tr( "Could not open file" ), QObject::tr( "Could not open file '%1' for write" ).arg( fn ) );
        return;
    }

    QTextStream ts( &file );

    QProgressDialog dlg( QObject::tr( "Saving Merged File '%1'..." ).arg( QFileInfo( fn ).fileName() ), "Cancel", 0, 0, parent );
    dlg.setMinimumDuration(0);
    dlg.setRange( 0, rowCount() );
    dlg.setValue(0);

    auto proxyModel = fTable.second.first->model();

    writeRow( ts, QStringList() << "No." << getHeader() );
    for ( int ii = 0; ii < proxyModel->rowCount(); ++ii )
    {
        if ( ( ii % 1000 ) == 0 )
            qApp->processEvents();
        if ( dlg.wasCanceled() )
            return;
        dlg.setValue( ii );

        QStringList rowData;
        for ( int jj = 0; jj < proxyModel->columnCount(); ++jj )
            rowData << proxyModel->index( ii, jj ).data().toString();
        writeRow( ts, QStringList() << QString::number( ii + 1 ) << rowData );
    }
}

void SFileData::writeRow( QTextStream & ts, QStringList & rowData ) const
{
    for ( auto && ii : rowData )
        ii = QString( R"("%1")" ).arg( ii );
    ts << rowData.join( "," ) << Qt::endl;
}

bool SFileData::loadFile( const QString & fileName, QWidget * parent )
{
    QFile fi( fileName );
    fi.open( QFile::Text | QFile::ReadOnly );
    if ( !fi.isOpen() )
    {
        QMessageBox::critical( parent, "Could not open", QString( "Error opening file '%1'" ).arg( fileName ) );
        return false;
    }

    QProgressDialog dlg( QObject::tr( "Loading File '%1'..." ).arg( fileName ), "Cancel", 0, 0, parent );
    dlg.setMinimumDuration( 0 );
    dlg.setValue( 1 );
    int lineNums = computeNumberOfLines( fi, &dlg );

    dlg.setRange( 0, lineNums );
    dlg.setValue( 1 );

    QTextStream ts( &fi );

    QString firstLine;
    while ( firstLine.isEmpty() )
    {
        firstLine = ts.readLine().trimmed();
    }
    auto header = getRow( firstLine );
    if ( !header.has_value() || !header.value().first || header.value().second.isEmpty() )
    {
        QMessageBox::critical( parent, "Could not open", QString( "Invalid format '%1' at Row: %2" ).arg( fileName ).arg( 1 ) );
        return false;
    }

    auto headerRow = header.value().second;
    TMergedType merged;
    for ( int ii = 0; ii < headerRow.count(); ++ii )
    {
        if ( headerRow[ ii ].toLower() == "first name" )
        {
            merged[ ii ] = { ii, 0 };
            if ( fMergedColumns )
                new QTreeWidgetItem( fMergedColumns, QStringList() << headerRow[ ii ] << "Name( 0 )" );
            headerRow[ ii ] = "Name";
        }
        else if ( headerRow[ ii ].toLower() == "remarks" )
        {
            fExtraUnimportantCols[ ii ] = QString();
        }
        else if ( headerRow[ ii ].toLower() == "call type" )
        {
            fExtraUnimportantCols[ ii ] = "Private Call";
        }
        else if ( headerRow[ ii ].toLower() == "call alert" )
        {
            fExtraUnimportantCols[ ii ] = "None";
        }
    }
    for ( auto && ii : fExtraUnimportantCols )
    {
        if ( fExtraColumns )
            new QListWidgetItem( QString( "%1(%2)" ).arg( headerRow[ ii.first ] ).arg( ii.first ), fExtraColumns );
    }
    if ( !merged.empty() )
    {
        for ( int ii = 0; ii < headerRow.count(); ++ii )
        {
            if ( headerRow[ ii ].toLower() == "last name" )
            {
                if ( fMergedColumns )
                    new QTreeWidgetItem( fMergedColumns, QStringList() << headerRow[ ii ] << "Name( 1 )" );

                auto firstPos = ( *merged.begin() ).second.first;
                merged[ ii ] = { firstPos, 1 };
                headerRow.removeAt( ii );
                break;
            }
        }
    }
    
    if ( fTable.first )
    {
        fTable.first->setColumnCount( headerRow.count() );
        fTable.first->setHorizontalHeaderLabels( headerRow );
        fTable.first->setRowCount( lineNums - 1 );
    }

    QString currLine;

    int rowNum = 0;
    int lineNum = 0;
    while ( ts.readLineInto( &currLine ) )
    {
        if ( ( rowNum % 1000 ) == 0 )
            qApp->processEvents();
        if ( dlg.wasCanceled() )
            return false;

#ifdef _DEBUG
        if ( rowNum >= 2000 )
            break;
#endif

        auto currRow = getRow( currLine, merged );
        if ( !currRow.has_value() ) // empty line after comments removed
            continue;
        if ( !currRow.value().first )
        {
            QMessageBox::critical( parent, "Could not open", QString( "Invalid format in file '%1' at Row: %2" ).arg( fileName ).arg( lineNum + 1) );
            return false;
        }

        lineNum++;
        auto currRowData = currRow.value().second;

        if ( isIgnoredRow( currRowData ) )
        {
            if ( fIgnoredRows )
                new QListWidgetItem( QString( "%1 - %2" ).arg( lineNum ).arg( currLine ), fIgnoredRows );
            continue;
        }
        if ( currRowData.count() != headerRow.count() )
        {
            QMessageBox::critical( parent, "Could not open", QString( "Invalid number of columns in file '%1' at Row: %2" ).arg( fileName ).arg( lineNum + 1 ) );
            return false;
        }
        for ( int ii = 0; ii < currRowData.count(); ++ii )
        {
            auto item = new QTableWidgetItem( currRowData[ ii ] );
            if ( fTable.first )
                fTable.first->setItem( rowNum, ii, item );
        }
        dlg.setValue( rowNum );
        ++rowNum;
    }
    if ( fTable.first )
        fTable.first->setRowCount( rowNum );
    computeHeaderInfo();
    if ( fTable.first )
        setTotalCount( fTable.first->rowCount() );

    return true;
}

bool SFileData::isIgnoredRow( const QStringList & currRowData ) const
{
    for ( auto && ii : currRowData )
    {
        if ( ii == "0" || ii.isEmpty() )
            continue;
        return false;
    }
    return true;
}

int SFileData::computeNumberOfLines( QFile & fi, QProgressDialog * dlg ) const
{
    int retVal = 0;
    QTextStream ts( &fi );
    while ( ts.readLineInto( nullptr ) )
    {
        if ( ( retVal % 1000 ) == 0 )
            qApp->processEvents();
        if ( dlg->wasCanceled() )
            return false;
#ifdef _DEBUG
        if ( retVal >= 2000 )
            break;
#endif

        retVal++;
    }

    fi.seek( 0 );
    return retVal;
}

bool SFileData::mergeData( SFileData & lhs, SFileData & rhs, SFileData & retVal, QWidget * parent )
{
    auto mergedModel = retVal.fTable.second.second;
    Q_ASSERT( mergedModel );
    if ( !mergedModel )
        return false;

    if ( !computeMD5s( lhs, rhs, parent ) )
    {
        return false;
    }

    QProgressDialog dlg( QObject::tr( "Merging Data..." ), "Cancel", 0, 0, parent );
    dlg.setRange( 0, lhs.rowCount() + rhs.rowCount() );
    dlg.setValue( 0 );
    dlg.setMinimumDuration( 0 );

    std::multimap< int, std::pair< int, int > > mergedData;
    // new row -> lhsrow, rhsRow

    int cnt = 0;
    for ( auto && ii : lhs.fRowToMD5 )
    {
        if ( ( cnt % 1000 ) == 0 )
            qApp->processEvents();
        if ( dlg.wasCanceled() )
            return false;
        dlg.setValue( cnt++ );

        auto && md5 = ii.second;
        auto pos = rhs.fMD5ToRow.find( md5 );
        if ( pos == rhs.fMD5ToRow.end() )
        {
            mergedData.insert( std::make_pair( ii.first, std::make_pair( ii.first, -1 ) ) );
        }
        else
        {
            mergedData.insert( std::make_pair( ii.first, std::make_pair( ii.first, ( *pos ).second ) ) );
        }
    }
    for ( auto && ii : rhs.fRowToMD5 )
    {
        if ( ( cnt % 1000 ) == 0 )
            qApp->processEvents();
        if ( dlg.wasCanceled() )
            return false;
        dlg.setValue( cnt++ );

        auto && md5 = ii.second;
        auto pos = lhs.fMD5ToRow.find( md5 );
        if ( pos == lhs.fMD5ToRow.end() )
        {
            mergedData.insert( std::make_pair( ii.first, std::make_pair( -1, ii.first ) ) );
        }
    }

    auto mergedDataCount = static_cast<int>( mergedData.size() );
    dlg.setLabelText( QObject::tr( "Loading Merged Data..." ) );
    dlg.setRange( 0, mergedDataCount );
    dlg.setValue( 0 );
    auto header = lhs.getColumns() + lhs.getExtraColumns() + rhs.getExtraColumns();
    if ( retVal.fTable.second.second )
    {
        retVal.fTable.second.second->setHeader( header );
    }
    int currRow = 0;
    int lhsOnlyCount = 0;
    int rhsOnlyCount = 0;
    int bothCount = 0;
    for ( auto && ii : mergedData )
    {
        if ( ( currRow % 1000 ) == 0 )
            qApp->processEvents();
        if ( dlg.wasCanceled() )
            return false;
        dlg.setValue( currRow );

        auto && currMergeInfo = ii.second;
        QStringList baseData;
        QStringList extraData;
        bool leftOnly = currMergeInfo.first != -1 && currMergeInfo.second == -1;
        bool rightOnly = currMergeInfo.first == -1 && currMergeInfo.second != -1;
        bool both = currMergeInfo.first != -1 && currMergeInfo.second != -1;
        if ( leftOnly || both )
        {
            baseData << lhs.getRowData( currMergeInfo.first );
            extraData << lhs.getExtraData( currMergeInfo.first );
        }
        else
        {
            extraData << lhs.getEmptyExtraData();
        }
        if ( rightOnly || both )
        {
            if ( rightOnly )
                baseData << rhs.getRowData( currMergeInfo.second );
            extraData << rhs.getExtraData( currMergeInfo.second );
        }
        else
            extraData << rhs.getEmptyExtraData();

        if ( leftOnly )
            lhsOnlyCount++;
        if ( rightOnly )
            rhsOnlyCount++;
        if ( both )
            bothCount++;
        
        auto rowData = baseData + extraData;
        for ( int jj = 0; jj < rowData.count(); ++jj )
        {
            if ( leftOnly )
            {
                lhs.setBackground( currMergeInfo.first, Qt::red );
            }
            else if ( rightOnly )
            {
                rhs.setBackground( currMergeInfo.second, Qt::yellow );
            }
        }
        mergedModel->addRow( rowData, leftOnly, rightOnly, false );
        currRow++;
    }
    mergedModel->modelReset();
    lhs.setSubCount( lhsOnlyCount );
    rhs.setSubCount( lhsOnlyCount );
    retVal.setSubCount( bothCount );
    retVal.setTotalCount( retVal.rowCount() );

    return true;
}

QString SFileData::getHeader( int pos ) const
{
    QString retVal;
    if ( fTable.first )
    {
        auto item = fTable.first->horizontalHeaderItem( pos );
        if ( !item )
            return {};
        retVal = item->text();
    }
    else if ( fTable.second.second )
    {
        retVal = fTable.second.second->headerData( pos, Qt::Horizontal ).toString();
    }

    if ( retVal.endsWith( "*" ) )
        retVal = retVal.left( retVal.length() - 1 );
    return retVal;
}

QStringList SFileData::getHeader() const
{
    QStringList retVal;

    for ( int ii = 0; ii < columnCount(); ++ii )
    {
        retVal << getHeader( ii );
    }
    return retVal;
}

int SFileData::columnCount() const
{
    int numColumns = 0;
    if ( fTable.first )
        numColumns = fTable.first->columnCount();
    else if ( fTable.second.second )
        numColumns = fTable.second.second->columnCount();
    return numColumns;
}

QString SFileData::itemText( int row, int col ) const
{
    if ( fTable.first )
    {
        auto item = fTable.first->item( row, col );
        if ( item )
            return item->text();
    }
    else if ( fTable.second.second )
    {
        auto idx = fTable.second.second->index( row, col );
        return idx.data().toString();
    }
    return QString();
}

int SFileData::rowCount() const
{
    int numRows = 0;
    if ( fTable.first )
        numRows = fTable.first->rowCount();
    else if( fTable.second.second )
        numRows = fTable.second.second->rowCount();
    return numRows;
}

QStringList SFileData::getColumns() const
{
    QStringList retVal;
    for ( auto && ii : fImportantCols )
    {
        retVal << getHeader( ii );
    }
    return retVal;
}

QStringList SFileData::getAllRowData( int row ) const
{
    if ( row >= rowCount() )
        return {};

    QStringList retVal;
    for ( int ii = 0; ii < columnCount(); ++ii )
    {
        retVal << getData( row, ii );
    }
    return retVal;
}

QStringList SFileData::getData( int row, const std::map< int, QString > & cols ) const
{
    if ( row >= rowCount() )
        return {};
    QStringList retVal;
    for ( auto && ii : cols )
    {
        retVal << getData( row, ii.first );
    }
    return retVal;
}

QStringList SFileData::getData( int row, const std::set< int > & cols ) const
{
    if ( row >= rowCount() )
        return {};
    QStringList retVal;
    for ( auto && ii : cols )
    {
        retVal << getData( row, ii );
    }
    return retVal;
}

QString SFileData::getData( int row, int col ) const
{
    QString retVal;
    if ( row == -1 )
        retVal = QString();
    else
    {
        retVal = itemText( row, col );
    }
    if ( retVal.isEmpty() )
    {
        auto pos = fExtraUnimportantCols.find( col );
        if ( pos != fExtraUnimportantCols.end() )
            retVal = ( *pos ).second;
    }
    return retVal;
}

void SFileData::setBackground( int row, Qt::GlobalColor clr )
{
    if ( row >= rowCount() )
        return;
    for ( int ii = 0; ii < columnCount(); ++ii )
    {
        if ( fTable.first )
        {
            auto item = fTable.first->item( row, ii );
            if ( item )
                item->setBackground( clr );
        }
    }
}

QStringList SFileData::getExtraColumns() const
{
    QStringList retVal;
    for ( auto && ii : fExtraUnimportantCols )
    {
        retVal << getHeader( ii.first );
    }
    return retVal;
}

bool SFileData::computeMD5s( const QString & label, QWidget * parent )
{
    QProgressDialog dlg( QObject::tr( "Computing %1 Values..." ).arg( label ), "Cancel", 0, 0, parent );
    dlg.setRange( 0, rowCount() );
    dlg.setMinimumDuration( 0 );

    int rowCount = this->rowCount();
    for ( int ii = 0; ii < rowCount; ++ii )
    {
        if ( ( ii % 1000 ) == 0 )
            qApp->processEvents();
        if ( dlg.wasCanceled() )
            return false;
        dlg.setValue( ii );

        QStringList importantData;
        for ( auto && jj : fImportantCols )
        {
            auto text = itemText( ii, jj );
            if ( !text.isEmpty() )
                importantData << text.left( 16 );
        }
        auto md5 = NSABUtils::getMd5( importantData );
        fMD5ToRow[ md5 ] = ii;
        fRowToMD5[ ii ] = md5; 
    }
    return true;
}

void SFileData::setTable( QTableView * view )
{
    fTable.second.first = view;
    fTable.second.second = new CMergedTableModel( view );
    auto proxy = new CMergedProxyModel( view );
    proxy->setSourceModel( fTable.second.second );
    view->setModel( proxy );
}

void SFileData::setTotalCount( int count )
{
    if ( fTotalCount )
        fTotalCount->setText( QString::number( count ) );
}

void SFileData::setSubCount( int count )
{
    if ( fSubCount )
        fSubCount->setText( QString::number( count ) );
}

void SFileData::updateMatchedColumns()
{
    if ( !fMatchedColumns )
        return;
    for ( auto && ii : fImportantCols )
    {
        new QListWidgetItem( QString( "%1(%2)" ).arg( getHeader( ii ) ).arg( ii ), fMatchedColumns );
    }
}

std::optional< std::pair< bool, QStringList > > SFileData::getRow( QString currLine, const TMergedType & mergedData ) const
{
    currLine = currLine.trimmed();
    if ( currLine.isEmpty() )
        return {};

    auto regExpStr = QString( R"((?:^|,)(?=[^"]|(")?)\"?((?(1).*?(?=\",)|[^,]*))(?=|$))" );
    auto regExp = QRegularExpression( regExpStr );

    QStringList retVal;
    QString currColumn;
    bool inQuote = false;
    for ( int ii = 0; ii < currLine.length(); ++ii )
    {
        auto curr = currLine[ ii ];
        if ( inQuote )
        {
            if ( curr == '"' )
            {
                for ( int jj = ii + 1; jj < currLine.length(); ++jj )
                {
                    if ( currLine[ jj ].isSpace() )
                        continue;
                    if ( currLine[ jj ] == ',' )
                    {
                        inQuote = false;
                        break;
                    }
                    break;
                }
            }
            else
                currColumn += curr;
        }
        else
        {
            if ( curr == ',' )
            {
                retVal << currColumn;
                currColumn.clear();
            }
            else if ( curr == '"' )
            {
                inQuote = true;
            }
            else
                currColumn += curr;
        }
    }
    retVal << currColumn;
    //auto ii = regExp.globalMatch( currLine );
    //if ( !ii.hasNext() )
    //    return { { false, QStringList() } };

    //QStringList retVal;
    //while ( ii.hasNext() )
    //{
    //    auto match = ii.next();
    //    auto text = match.captured( 2 );
    //    retVal << text;
    //}

    if ( !mergedData.empty() )
    {
        std::map< int, QStringList > realRetVal;
        for ( int ii = 0; ii < retVal.count(); ++ii )
        {
            auto pos = mergedData.find( ii );
            if ( pos == mergedData.end() )
            {
                realRetVal[ ii ] = QStringList( { retVal[ ii ] } );
            }
            else
            {
                auto newCol = ( *pos ).second.first;
                auto posInList = ( *pos ).second.second;
                auto pos2 = realRetVal.find( newCol );
                QStringList tmp;
                if ( pos2 != realRetVal.end() )
                {
                    tmp = ( *pos2 ).second;
                }

                while ( tmp.count() <= posInList )
                {
                    tmp << QString();
                }

                tmp[ posInList ] = retVal[ ii ];
                realRetVal[ newCol ] = tmp;
            }
        }
        retVal.clear();
        for ( auto && ii : realRetVal )
        {
            retVal.push_back( ii.second.join( " " ).trimmed() );
        }
    }

    return { { true, retVal } };
}

void SFileData::computeHeaderInfo()
{
    for ( int ii = 0; ii < columnCount(); ++ii )
    {
        QTableWidgetItem * item = nullptr;
        if ( fTable.first )
        {
            item = fTable.first->horizontalHeaderItem( ii );
        }
        fHeaderInfo[ getHeader( ii ) ] = { item, ii };
    }
}

bool SFileData::computeMD5s( SFileData & lhs, SFileData & rhs, QWidget * parent )
{
    for ( auto && ii : lhs.fHeaderInfo )
    {
        auto pos = rhs.fHeaderInfo.find( ii.first );
        if ( pos != rhs.fHeaderInfo.end() )
        {
            auto newName = ( ii.second.first->text() + "*" );

            lhs.fImportantCols.insert( ii.second.second );
            rhs.fImportantCols.insert( ( *pos ).second.second );

            ii.second.first->setText( newName );
            ( *pos ).second.first->setText( newName );
        }
    }

    auto aOK = lhs.computeMD5s( "LHS", parent );
    aOK = aOK && rhs.computeMD5s( "RHS", parent );
    return aOK;
}

void CMainWindow::slotResultsItemChanged( QTreeWidgetItem * curr, QTreeWidgetItem * /*prev*/ )
{
    if ( !curr )
        fImpl->resultsPages->setCurrentIndex( 0 );
    else if ( curr->text( 0 ) == "Summary" )
        fImpl->resultsPages->setCurrentIndex( 0 );
    else if ( curr->text( 0 ) == "Ignored Rows" )
        fImpl->resultsPages->setCurrentIndex( 1 );
    else if ( curr->text( 0 ) == "Merged Columns" )
        fImpl->resultsPages->setCurrentIndex( 2 );
    else if ( curr->text( 0 ) == "Extra Columns" )
        fImpl->resultsPages->setCurrentIndex( 3 );
    else if ( curr->text( 0 ) == "Matched Columns" )
        fImpl->resultsPages->setCurrentIndex( 4 );
}

CMergedTableModel::CMergedTableModel( QObject * parent ) :
    QAbstractTableModel( parent )
{

}

void CMergedTableModel::clear()
{
    beginResetModel();
    fHeaderInfo.clear();
    fData.clear();
    endResetModel();
}

bool CMergedProxyModel::lessThan( const QModelIndex & lhs, const QModelIndex & rhs ) const
{
    if ( lhs.column() == 0 )
    {
        return lhs.data().toInt() < rhs.data().toInt();
    }
    return QSortFilterProxyModel::lessThan( lhs, rhs );
}
