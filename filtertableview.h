#ifndef FILTERTABLEVIEW_H
#define FILTERTABLEVIEW_H

#include <QHeaderView>
#include <QTableView>
#include <QScrollArea>
#include <QWidget>

#include <QTimer>
#include <QScrollBar>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QCompleter>

#include <QVBoxLayout>

#include "Models/csortfilterproxymodel.h"

//#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

#include <QDebug>

#include <QLabel>

class FilterHeader : public QHeaderView
{
    Q_OBJECT
public:
    explicit FilterHeader(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setFilterBoxes(const int columnCount);

    QSize sizeHint() const override;

    void adjustPositions();

    QScrollArea *filterScrollArea;

protected:
    void updateGeometries() override;
//    QSize sectionSizeFromContents(int logicalIndex) const override;

private:
    QVector<QLineEdit*> editors;
    int padding;
    QTimer *timer;

    QWidget *filterProxyWidget;
    QHBoxLayout *hLayout;
    QVBoxLayout *filterLayout;

signals:
    void filterTViewTextChanged(const int column, const QString text);

};

class FilterTableView : public QTableView
{
    Q_OBJECT

public:
    explicit FilterTableView(QWidget *parent = nullptr);

    QModelIndex currentIndex() const;

    void setRegExpForColumn(QMap<int, QString> regexpColumns);
    void setEditColumns(QVector<int> editColumns);

    QMap<int, QString> getRegExpColumns();
    QVector<int> getExcludeColumns();

    /**
     * setModel с установкой SortFilter модели при включенной фильтрацией (по умолчанию включена)
     */
    void setModel(QAbstractItemModel *model) override;
    QModelIndex modelIndex(const int row, const int column, const QModelIndex &parent = QModelIndex());

    QMap<int, QString> *getGetRegEpColumns() const;
    void setGetRegEpColumns(QMap<int, QString> *value);

    bool getFiltersEnabled() const;
    void setFiltersEnabled(bool newFiltersEnabled);
    void resetFiltersEnabled();

    void resetFilters();
    void resetHeader();

    QByteArray saveState();
    void restoreState(const QByteArray &state);

    const QList<int> &getHiddenColumns() const;
    void setHiddenColumns(const QList<int> &newHiddenColumns);

    void moveSection(int from, int to);
    void swapSections(int first, int second);

    void setColumnHidden(int column, bool hide);

private:
    QAbstractItemModel *_model;
    QMap<int, QString> regExpColumns;
    QVector<int> excludeColumns;
    FilterHeader *customHeader;

    bool filtersEnabled;
    CSortFilterProxyModel *proxyModel;
    QItemSelectionModel *itemSelectionModel;

    QList<int> hiddenColumns;

    Q_PROPERTY(bool filtersEnabled READ getFiltersEnabled WRITE setFiltersEnabled RESET resetFiltersEnabled NOTIFY filtersEnabledChanged)

signals:
//    void filterTViewTextChanged(const int column, const QString text);
    void filtersEnabledChanged();

private slots:
    void setFilter(const int column, const QString filterPattern);    
    void sectionResized(int logicalIndex, int oldSize, int newSize);

};

#endif // FILTERTABLEVIEW_H
