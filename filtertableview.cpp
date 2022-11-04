#include "filtertableview.h"

FilterHeader::FilterHeader(Qt::Orientation orientation, QWidget *parent):
    QHeaderView(orientation, parent),
    editors({}),
    padding(4)
{
    timer = new QTimer(this);

//    setStretchLastSection(true);
//    setSectionResizeMode(QHeaderView::Stretch);

    setSectionResizeMode(QHeaderView::Interactive);
    setDefaultAlignment(Qt::AlignLeft | Qt::AlignCenter);
//    setDefaultAlignment(Qt::AlignLeft | (Qt::Alignment)Qt::TextWordWrap);
    setSortIndicatorShown(true);
    setSectionsClickable(true);

    connect(this, &FilterHeader::sectionResized, this, &FilterHeader::adjustPositions);
//    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &FilterHeader::adjustPositions);

    hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(0);

    filterScrollArea = new QScrollArea(this);
    filterScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    filterScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    filterScrollArea->setFixedHeight(20);
    filterScrollArea->horizontalScrollBar()->setStyleSheet("QScrollBar {height:0px;}");
    filterProxyWidget = new QWidget(this);
    filterProxyWidget->setFixedHeight(20);
    filterScrollArea->setWidgetResizable(true);
    filterScrollArea->setWidget(filterProxyWidget);

    filterProxyWidget->setLayout(hLayout);

    filterLayout = new QVBoxLayout(this);
    filterLayout->setMargin(0);
    filterLayout->addSpacerItem(new QSpacerItem(1, 21));
    filterLayout->addWidget(filterScrollArea);

    setLayout(filterLayout);
}

void FilterHeader::setFilterBoxes(const int columnCount)
{
    QMap<int, QString> regexpColumns = static_cast<FilterTableView*>(parent())->getRegExpColumns();

//    qDeleteAll(filterLayout->findChildren<QWidget *>(Qt::FindChildrenRecursively));

    // чистим список фильтров
    qDeleteAll(editors);
    editors.clear();

    for (int i = 0; i < hLayout->count(); ++i) {
        QLayoutItem *layoutItem = hLayout->itemAt(i);
        if (layoutItem->spacerItem()) {
            hLayout->removeItem(layoutItem);
            // You could also use: layout->takeAt(i);
            delete layoutItem;
            --i;
        }
    }
//    qDebug() << "trying to add filters" << columnCount;

    for (int i = 0 ; i < columnCount; ++i)
    {
        //Если массив исключений содержит столбцы, которым не нужно добавлять QLineEdit
        if (static_cast<FilterTableView*>(parent())->getExcludeColumns().contains(i))
            continue;

        QLineEdit *le = new QLineEdit(this);
        le->setPlaceholderText(QString("фильтр")/* + QString::number(i)*/);
        le->setClearButtonEnabled(true);

        // TODO добавить QCompleter в фильтры
//        QCompleter *completer = new QCompleter();
//        completer->setCaseSensitivity(Qt::CaseInsensitive);
//        completer->setCompletionMode(QCompleter::PopupCompletion);
//        completer->setModel(model);
//        le->setCompleter(completer);

        //Если массив проверки валидации для добавляемого QLineEdit столбца содержит правила валидации
        if (regexpColumns.contains(i))
            le->setValidator(new QRegularExpressionValidator(QRegularExpression(regexpColumns.value(i))));

        editors.append(le);
//        filterLayout->addWidget(le);
        hLayout->addWidget(le);

        //Данный коннект делает задержку фильтрации на 1 секунду
        connect(le, &QLineEdit::textChanged, [this, i](const QString text)
        {
            timer->start(1000);
            connect(timer, &QTimer::timeout, [this, i, text](){
                timer->stop(); emit filterTViewTextChanged(i, text); });
        });

//        qDebug() << "filter add" << i;

    }

    if (columnCount > 0)
    {
        QSpacerItem *s = new QSpacerItem(1, 20, QSizePolicy::Expanding);
        hLayout->addSpacerItem(s);
    }

    adjustPositions();
}

QSize FilterHeader::sizeHint() const
{
    QSize size = QHeaderView::sizeHint();
    if(!editors.isEmpty())
    {
        const int height = editors[0]->sizeHint().height();
        size.setHeight(size.height() + height + padding);
    }
    return size;
}

void FilterHeader::updateGeometries()
{
    if(!editors.isEmpty())
    {
        const int height = editors[0]->sizeHint().height();
        setViewportMargins(0, 0, 0, height + padding);
    }
    else
        setViewportMargins(0, 0, 0, 0);

    QHeaderView::updateGeometries();

    adjustPositions();
}

//QSize FilterHeader::sectionSizeFromContents(int logicalIndex) const
//{
//    const QString text = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
//    const int maxWidth = sectionSize(logicalIndex);
//    const int maxHeight = 5000;

//    const Qt::Alignment alignment = defaultAlignment();
//    const QFontMetrics metrics(fontMetrics());

//    const QRect rect = metrics.boundingRect(QRect(0, 0, maxWidth, maxHeight), alignment, text);
//    const QSize textMarginBuffer(2, 2);

//    return rect.size() + textMarginBuffer;
//}

void FilterHeader::adjustPositions()
{
    //Метод изменения позиции QLineEdit
    int column = 0;
    int width = 0;

    foreach(QLineEdit *le, editors)
    {
//        const int height = editors[column]->sizeHint().height();
//        le->move(sectionPosition(column) - offset(), height + padding);
//        le->resize(sectionSize(column), height);
        le->setFixedWidth(sectionSize(column));
        ++column;

        width += sectionSize(column);
    }

    if (width != 0)
        static_cast<FilterTableView*>(parent())->setMinimumWidth(width);
//        static_cast<FilterTableView*>(parent())->setFixedWidth(width);
}


FilterTableView::FilterTableView(QWidget *parent) :
    QTableView(parent),
//    filterLayout(nullptr),
    _model(nullptr),
    customHeader(nullptr),
    filtersEnabled(true),
    proxyModel(nullptr),
    itemSelectionModel(nullptr),
    hiddenColumns(QList<int>())
{
    customHeader = new FilterHeader(Qt::Horizontal, this);
    setHorizontalHeader(customHeader);

//    horizontalHeader()->setLayout(filterLayout);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Interactive);

    connect(customHeader, &FilterHeader::filterTViewTextChanged, this, &FilterTableView::setFilter);
//    connect(customHeader, &FilterHeader::sectionResized, this, &FilterTableView::sectionResized);

    connect (horizontalScrollBar(), &QScrollBar::valueChanged, customHeader->filterScrollArea->horizontalScrollBar(), &QScrollBar::setValue);

    connect (horizontalScrollBar(), &QScrollBar::rangeChanged, customHeader->filterScrollArea->horizontalScrollBar(), &QScrollBar::setRange);

//    connect(horizontalScrollBar(), &QScrollBar::valueChanged, [this]() { qDebug() << "table bar value" << horizontalScrollBar()->value() << horizontalScrollBar()->maximum(); });

//    connect (horizontalScrollBar(), &QScrollBar::sliderMoved, this, [this](){resetHeader();});

}

QModelIndex FilterTableView::currentIndex() const
{
    if (filtersEnabled)
    {
        if (proxyModel != nullptr)
            return proxyModel->mapToSource(QTableView::currentIndex());
        else
            return QModelIndex();
    }
    else
        return QTableView::currentIndex();
}

void FilterTableView::setModel(QAbstractItemModel *model)
{
    _model = model;
//    customHeader->setFilterBoxes(_model->columnCount(), hLayout, model);

    // фильтрация - если включено, то при необходимости создаем сорт модель
    // и в зависимости от состояния фильтра устанавливаем нужную модель во view
    if (filtersEnabled)
    {
        if (proxyModel == nullptr)
            proxyModel = new CSortFilterProxyModel(this);

        proxyModel->setSourceModel(_model);

        QTableView::setModel(proxyModel);
    }
    else
        QTableView::setModel(_model);

    customHeader->setFilterBoxes(_model->columnCount());

    // TODO задать ширину столбцов и их видимость
}

QModelIndex FilterTableView::modelIndex(const int row, const int column, const QModelIndex &parent)
{
    if (filtersEnabled)
        return proxyModel->index(row, column, parent);
    else
        return _model->index(row, column, parent);
}

bool FilterTableView::getFiltersEnabled() const
{
    return filtersEnabled;
}

void FilterTableView::setFiltersEnabled(bool newFiltersEnabled)
{
    if (filtersEnabled == newFiltersEnabled)
        return;

    if (newFiltersEnabled)
        setModel(_model);

    filtersEnabled = newFiltersEnabled;
    emit filtersEnabledChanged();
}

void FilterTableView::resetFiltersEnabled()
{
    setFiltersEnabled(true);
}

void FilterTableView::resetFilters()
{
    if (filtersEnabled)
        customHeader->setFilterBoxes(_model->columnCount());
//        customHeader->setFilterBoxes(_model->columnCount(), hLayout, _model);
}

void FilterTableView::resetHeader()
{
    customHeader->adjustPositions();
}

QByteArray FilterTableView::saveState()
{
    return customHeader->saveState();
}

void FilterTableView::restoreState(const QByteArray &state)
{
    customHeader->restoreState(state);
}

const QList<int> &FilterTableView::getHiddenColumns() const
{
    return hiddenColumns;
}

void FilterTableView::setHiddenColumns(const QList<int> &newHiddenColumns)
{
    hiddenColumns = newHiddenColumns;
}

void FilterTableView::moveSection(int from, int to)
{
    customHeader->moveSection(from, to);
}

void FilterTableView::swapSections(int first, int second)
{
    customHeader->swapSections(first, second);
}

void FilterTableView::setColumnHidden(int column, bool hide)
{
    QTableView::setColumnHidden(column, hide);

    if (hide)
        excludeColumns.append(column);
    else
        excludeColumns.remove(column);
//    if (filtersEnabled)
//        proxyModel->addHiddenColumn(column, hide);
}

void FilterTableView::setFilter(const int column, const QString filterPattern)
{
    // обработка фильтра - не зависит отрегистра, задаем колонку и шаблон

//    qDebug() << __FUNCTION__ << column << filterPattern;

    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(column);
    proxyModel->addFilterFixedString(column, filterPattern);
//    proxyModel->setFilterFixedString(filterPattern);

    customHeader->adjustPositions();

//    proxyModel->invalidate();
    proxyModel->callInvalidateFilter();
}

void FilterTableView::sectionResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(logicalIndex);
    Q_UNUSED(oldSize);
    Q_UNUSED(newSize);

    resetHeader();
}

void FilterTableView::setRegExpForColumn(QMap<int, QString> regexpColumns)
{
    regExpColumns = regexpColumns;
}

void FilterTableView::setEditColumns(QVector<int> editColumns)
{
    excludeColumns = editColumns;
}

QMap<int, QString> FilterTableView::getRegExpColumns()
{
    return regExpColumns;
}

QVector<int> FilterTableView::getExcludeColumns()
{
    return excludeColumns;
}
