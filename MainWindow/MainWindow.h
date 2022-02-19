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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>
#include <unordered_map>
#include <optional>
#include <set>

class QTableWidget;
class QTableView;
class QTableWidgetItem;
class QTextStream;
class QProgressDialog;
class QFile;
class QTreeWidgetItem;
class QLineEdit;
class QTreeWidget;
class QListWidget;

class CMergedTableModel;
namespace Ui {class CMainWindow;};
struct SFileData
{
    void clear();
    bool loadFile( const QString & fileName, QWidget * parent );

    void save( QWidget * parent );

    static bool mergeData( SFileData & lhs, SFileData & rhs, SFileData & retVal, QWidget * parent );

    void setTable( QTableWidget * table ) { fTable.first = table; }
    void setTable( QTableView * view );
    void setTotalCount( QLineEdit * le ) { fTotalCount = le; }
    void setSubCount( QLineEdit * le ) { fSubCount = le; }
    void setMergedColumns( QTreeWidget * tree ) { fMergedColumns = tree; }
    void setExtraColumns( QListWidget * list ) { fExtraColumns = list; }
    void setMatchedColumns( QListWidget * list ) { fMatchedColumns = list; }
    void setIgnoredRows( QListWidget * list ) { fIgnoredRows = list; }

    int rowCount() const;
    int columnCount() const;
    QString itemText( int row, int col ) const;

    int numImportantColumns() const { return static_cast< int >( fImportantCols.size() ); }
    void setTotalCount( int count );
    void setSubCount( int count );

    void updateMatchedColumns();
private:
    bool isIgnoredRow( const QStringList & currRowData ) const;
    int computeNumberOfLines( QFile & fi, QProgressDialog * dlg ) const;
    void writeRow( QTextStream & ts, QStringList & rowData ) const;
    QStringList getRowData( int row ) const
    {
        return getData( row, fImportantCols );
    }
    QStringList getExtraData( int row ) const
    {
        return getData( row, fExtraUnimportantCols );
    }
    QStringList getEmptyExtraData() const
    {
        return getData( -1, fExtraUnimportantCols );
    }
    QStringList getData( int row, const std::set< int > & cols ) const;
    QStringList getData( int row, const std::map< int, QString > & cols ) const;
    QStringList getAllRowData( int row ) const;
    QString getData( int row, int col ) const;

    void setBackground( int row, Qt::GlobalColor clr );

    QStringList getExtraColumns() const;
    QStringList getColumns() const;

    QString getHeader( int headerCol ) const;
    QStringList getHeader() const;

    static bool computeMD5s( SFileData & lhs, SFileData & rhs, QWidget * parent );
    bool computeMD5s( const QString & label, QWidget * parent );
    using TMergedType = std::unordered_map< int, std::pair< int, int > >;
    std::optional< std::pair< bool, QStringList > > getRow( QString currLine, const TMergedType & mergedInfo = {} ) const;

    void computeHeaderInfo();
    std::pair< QTableWidget *, std::pair< QTableView *, CMergedTableModel * > > fTable{ nullptr, { nullptr, nullptr} };
    QLineEdit * fTotalCount{ nullptr };
    QLineEdit * fSubCount{ nullptr };
    QTreeWidget * fMergedColumns{ nullptr };
    QListWidget * fExtraColumns{ nullptr };
    QListWidget * fMatchedColumns{ nullptr };
    QListWidget * fIgnoredRows{ nullptr };
    std::map< int, QByteArray > fRowToMD5;
    std::unordered_map< QByteArray, int > fMD5ToRow;
    std::map< QString, std::pair< QTableWidgetItem *, int > > fHeaderInfo;
    std::map< int, QString > fExtraUnimportantCols;
    std::set< int > fImportantCols;
};

class CMergedTableModel : public QAbstractTableModel
{
    Q_OBJECT;
public:
    CMergedTableModel( QObject * parent );

    void clear();
    void modelReset()
    {
        beginResetModel();
        endResetModel();
    }

    void setHeader( const QStringList & headerInfo )
    {
        beginResetModel();
        fHeaderInfo = headerInfo;
        endResetModel();
    }
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override
    {
        if ( ( orientation != Qt::Orientation::Horizontal ) || ( role != Qt::DisplayRole ) )
            return QAbstractTableModel::headerData( section, orientation, role );
        if ( section >= fHeaderInfo.count() )
            return section;
        return fHeaderInfo[ section ];
    }

    virtual int columnCount( const QModelIndex & /*idx*/ = QModelIndex() ) const override
    {
        return fHeaderInfo.count();
    }

    virtual int rowCount( const QModelIndex & /*idx*/ = QModelIndex() ) const override
    {
        return static_cast< int >( fData.size() );;
    }

    virtual void addRow( const QStringList & rowData, bool leftOnly, bool rightOnly, bool emitSignal )
    {
        if ( emitSignal )
            beginInsertRows( QModelIndex(), static_cast<int>( fData.size() ), static_cast<int>( fData.size() ) );
        fData.emplace_back( std::make_tuple( rowData, leftOnly, rightOnly ) );
        if ( emitSignal )
            endInsertRows();
    }
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override
    {
        if ( !index.isValid() )
            return {};
        if ( index.row() >= rowCount() )
            return {};
        if ( index.column() >= columnCount() )
            return {};
        if ( role == Qt::DisplayRole )
            return std::get< 0 >( fData[ index.row() ] )[ index.column() ];
        else if ( role == Qt::BackgroundRole )
        {
            if ( std::get< 1 >( fData[ index.row() ] ) )
                return QBrush( Qt::red );
            if ( std::get< 2 >( fData[ index.row() ] ) )
                return QBrush( Qt::yellow );
        }
        return QVariant();
    }
private:
    QStringList fHeaderInfo;
    std::vector< std::tuple< QStringList, bool, bool > > fData;
};

class CMergedProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    CMergedProxyModel( QObject * parent ) :
        QSortFilterProxyModel( parent )
    {
    }
protected:
    bool lessThan( const QModelIndex & lhs, const QModelIndex & rhs ) const override;
};

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum ENodeType
    {
        eParentDir = 1000,
        eOK,
        eMissingDir,
        eOKDirToRename,
        eBadFileName
    };

    CMainWindow(QWidget* parent = 0);
    ~CMainWindow() override;
public Q_SLOTS:
    void slotSelectRHSFile();
    void slotSelectLHSFile();
    void slotFilesChanged();
    void slotLoad();
    void slotSave();

    void slotResultsItemChanged( QTreeWidgetItem * curr, QTreeWidgetItem * prev );
private:
    void loadSettings();
    void saveSettings();
    void loadFiles();

    void clear();

    SFileData fLHS;
    SFileData fRHS;
    SFileData fMerged;

    std::unique_ptr< Ui::CMainWindow > fImpl;
};



#endif 
