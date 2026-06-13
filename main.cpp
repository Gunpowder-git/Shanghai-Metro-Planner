#include <QtWidgets>
#include <queue>
#include <climits>
#include <vector>

// ============================================================
// 上海地铁换乘指南系统
// Qt Widgets + C++17
// 功能：
// 1. 图形化显示地铁网络结构
// 2. 动态添加地铁线路和地铁站点
// 3. 根据起点站、终点站查询换乘指南
// 4. 在图形界面中高亮显示乘车路径
// 5. 初始地铁图按用户提供的上海地铁图做了扩充版模拟
// ============================================================

struct Edge {
    QString to;
    QString line;
};

struct Station {
    QString name;
    QPointF pos;
    QSet<QString> lines;
};

struct RouteState {
    QString station;
    QString line;
};

struct RouteResult {
    QVector<RouteState> states;
    int transfers = 0;
    int stops = 0;
    QString error;
};

class MetroGraph {
public:
    void addLine(const QString &line, const QColor &color) {
        if (line.trimmed().isEmpty()) return;
        if (!lineColors.contains(line)) {
            lineColors[line] = color;
            lineNames.append(line);
            lineStationOrder[line] = QStringList();
        }
    }

    void addStation(const QString &line,
                    const QString &station,
                    const QPointF &pos,
                    const QString &previousStation = QString()) {
        if (line.trimmed().isEmpty() || station.trimmed().isEmpty()) return;

        if (!lineColors.contains(line)) {
            addLine(line, colorByIndex(lineNames.size()));
        }

        if (!stations.contains(station)) {
            Station s;
            s.name = station;
            s.pos = pos;
            stations[station] = s;
        }

        stations[station].lines.insert(line);

        if (!lineStationOrder[line].contains(station)) {
            lineStationOrder[line].append(station);
        }

        if (!previousStation.trimmed().isEmpty() && stations.contains(previousStation)) {
            addUndirectedEdge(previousStation, station, line);
        }
    }

    bool hasStation(const QString &name) const {
        return stations.contains(name);
    }

    QStringList allStations() const {
        QStringList names = stations.keys();
        names.sort(Qt::CaseInsensitive);
        return names;
    }

    QStringList allLines() const {
        return lineNames;
    }

    QStringList stationsOfLine(const QString &line) const {
        return lineStationOrder.value(line);
    }

    const QMap<QString, Station>& getStations() const {
        return stations;
    }

    const QMap<QString, QVector<Edge>>& getAdj() const {
        return adj;
    }

    QColor colorOfLine(const QString &line) const {
        return lineColors.value(line, Qt::darkGray);
    }

    RouteResult shortestRoute(const QString &start, const QString &target) const {
        RouteResult result;

        if (!stations.contains(start)) {
            result.error = QString("起点站不存在：%1").arg(start);
            return result;
        }
        if (!stations.contains(target)) {
            result.error = QString("终点站不存在：%1").arg(target);
            return result;
        }
        if (start == target) {
            result.states.append({start, QString()});
            return result;
        }

        struct Cost {
            int transfers = INT_MAX;
            int stops = INT_MAX;
        };

        auto better = [](const Cost &a, const Cost &b) {
            if (a.transfers != b.transfers) return a.transfers < b.transfers;
            return a.stops < b.stops;
        };

        struct Node {
            QString key;
            int transfers;
            int stops;
        };

        struct Cmp {
            bool operator()(const Node &a, const Node &b) const {
                if (a.transfers != b.transfers) return a.transfers > b.transfers;
                return a.stops > b.stops;
            }
        };

        QMap<QString, Cost> dist;
        QMap<QString, QString> prev;
        std::priority_queue<Node, std::vector<Node>, Cmp> pq;

        QString startKey = makeKey(start, QString());
        dist[startKey] = {0, 0};
        pq.push({startKey, 0, 0});

        while (!pq.empty()) {
            Node cur = pq.top();
            pq.pop();

            Cost known = dist.value(cur.key, {INT_MAX, INT_MAX});
            if (cur.transfers != known.transfers || cur.stops != known.stops) continue;

            auto parts = splitKey(cur.key);
            QString curStation = parts.first;
            QString curLine = parts.second;

            if (!adj.contains(curStation)) continue;

            for (const Edge &e : adj.value(curStation)) {
                int addTransfer = 0;
                if (!curLine.isEmpty() && curLine != e.line) {
                    addTransfer = 1;
                }

                Cost nextCost{cur.transfers + addTransfer, cur.stops + 1};
                QString nextKey = makeKey(e.to, e.line);

                if (!dist.contains(nextKey) || better(nextCost, dist[nextKey])) {
                    dist[nextKey] = nextCost;
                    prev[nextKey] = cur.key;
                    pq.push({nextKey, nextCost.transfers, nextCost.stops});
                }
            }
        }

        QString bestKey;
        Cost bestCost;
        for (auto it = dist.begin(); it != dist.end(); ++it) {
            auto parts = splitKey(it.key());
            if (parts.first == target) {
                if (bestKey.isEmpty() || better(it.value(), bestCost)) {
                    bestKey = it.key();
                    bestCost = it.value();
                }
            }
        }

        if (bestKey.isEmpty()) {
            result.error = QString("%1 到 %2 暂无可达路径").arg(start, target);
            return result;
        }

        QVector<QString> keys;
        QString k = bestKey;
        while (!k.isEmpty()) {
            keys.prepend(k);
            if (!prev.contains(k)) break;
            k = prev[k];
        }

        for (const QString &key : keys) {
            auto parts = splitKey(key);
            result.states.append({parts.first, parts.second});
        }
        result.transfers = bestCost.transfers;
        result.stops = bestCost.stops;
        return result;
    }

    static QColor colorByIndex(int index) {
        static QVector<QColor> colors = {
            QColor(225, 40, 40), QColor(70, 185, 60), QColor(250, 210, 0),
            QColor(95, 65, 165), QColor(160, 90, 210), QColor(230, 40, 150),
            QColor(250, 130, 40), QColor(70, 170, 220), QColor(100, 175, 90),
            QColor(205, 80, 180), QColor(145, 40, 70), QColor(40, 160, 80),
            QColor(245, 130, 190), QColor(150, 145, 60), QColor(170, 150, 220),
            QColor(60, 190, 190), QColor(170, 170, 170), QColor(190, 135, 75)
        };
        return colors[index % colors.size()];
    }

private:
    QMap<QString, Station> stations;
    QMap<QString, QVector<Edge>> adj;
    QMap<QString, QColor> lineColors;
    QMap<QString, QStringList> lineStationOrder;
    QStringList lineNames;

    void addUndirectedEdge(const QString &a, const QString &b, const QString &line) {
        addDirectedEdge(a, b, line);
        addDirectedEdge(b, a, line);
    }

    void addDirectedEdge(const QString &from, const QString &to, const QString &line) {
        for (const Edge &e : adj[from]) {
            if (e.to == to && e.line == line) return;
        }
        adj[from].append({to, line});
    }

    static QString makeKey(const QString &station, const QString &line) {
        return station + QChar(0x1F) + line;
    }

    static QPair<QString, QString> splitKey(const QString &key) {
        int p = key.indexOf(QChar(0x1F));
        if (p < 0) return {key, QString()};
        return {key.left(p), key.mid(p + 1)};
    }
};

class MainWindow : public QMainWindow {
public:
    MainWindow() {
        setWindowTitle("上海地铁换乘指南系统");
        resize(1400, 850);

        scene = new QGraphicsScene(this);
        view = new QGraphicsView(scene);
        view->setRenderHint(QPainter::Antialiasing);
        view->setDragMode(QGraphicsView::ScrollHandDrag);
        view->setMinimumWidth(920);
        view->setBackgroundBrush(QBrush(QColor(34, 39, 46)));

        QWidget *central = new QWidget;
        QHBoxLayout *mainLayout = new QHBoxLayout(central);
        mainLayout->addWidget(view, 1);
        mainLayout->addWidget(buildControlPanel(), 0);
        setCentralWidget(central);

        buildShanghaiMetro();
        refreshAllComboBoxes();
        drawNetwork();
    }

private:
    MetroGraph graph;
    QGraphicsScene *scene = nullptr;
    QGraphicsView *view = nullptr;

    QComboBox *startBox = nullptr;
    QComboBox *targetBox = nullptr;
    QComboBox *lineBox = nullptr;
    QComboBox *prevStationBox = nullptr;

    QLineEdit *newLineEdit = nullptr;
    QLineEdit *stationNameEdit = nullptr;
    QLineEdit *xEdit = nullptr;
    QLineEdit *yEdit = nullptr;
    QTextEdit *resultText = nullptr;

    using StationPoint = QPair<QString, QPointF>;

    QWidget* buildControlPanel() {
        QWidget *panel = new QWidget;
        panel->setFixedWidth(410);
        QVBoxLayout *layout = new QVBoxLayout(panel);

        QLabel *title = new QLabel("上海地铁换乘指南");
        QFont titleFont = title->font();
        titleFont.setPointSize(16);
        titleFont.setBold(true);
        title->setFont(titleFont);
        layout->addWidget(title);

        QGroupBox *queryGroup = new QGroupBox("查询乘车路径");
        QFormLayout *queryLayout = new QFormLayout(queryGroup);
        startBox = new QComboBox;
        targetBox = new QComboBox;
        startBox->setEditable(true);
        targetBox->setEditable(true);
        QPushButton *queryBtn = new QPushButton("生成换乘指南并高亮路径");
        queryLayout->addRow("起点站：", startBox);
        queryLayout->addRow("终点站：", targetBox);
        queryLayout->addRow(queryBtn);
        layout->addWidget(queryGroup);

        connect(queryBtn, &QPushButton::clicked, this, [this]() {
            QString start = startBox->currentText().trimmed();
            QString target = targetBox->currentText().trimmed();
            RouteResult route = graph.shortestRoute(start, target);
            drawNetwork(route.states);
            resultText->setPlainText(makeGuideText(route, start, target));
        });

        QGroupBox *lineGroup = new QGroupBox("动态添加地铁线路");
        QHBoxLayout *lineLayout = new QHBoxLayout(lineGroup);
        newLineEdit = new QLineEdit;
        newLineEdit->setPlaceholderText("例如：19号线");
        QPushButton *addLineBtn = new QPushButton("添加线路");
        lineLayout->addWidget(newLineEdit);
        lineLayout->addWidget(addLineBtn);
        layout->addWidget(lineGroup);

        connect(addLineBtn, &QPushButton::clicked, this, [this]() {
            QString line = newLineEdit->text().trimmed();
            if (line.isEmpty()) {
                QMessageBox::warning(this, "提示", "线路名不能为空。");
                return;
            }
            graph.addLine(line, MetroGraph::colorByIndex(graph.allLines().size()));
            newLineEdit->clear();
            refreshAllComboBoxes();
            drawNetwork();
        });

        QGroupBox *stationGroup = new QGroupBox("动态添加地铁站点");
        QFormLayout *stationLayout = new QFormLayout(stationGroup);
        lineBox = new QComboBox;
        prevStationBox = new QComboBox;
        stationNameEdit = new QLineEdit;
        xEdit = new QLineEdit;
        yEdit = new QLineEdit;
        QPushButton *addStationBtn = new QPushButton("添加站点");

        stationNameEdit->setPlaceholderText("例如：新站点");
        xEdit->setPlaceholderText("例如：650");
        yEdit->setPlaceholderText("例如：430");

        stationLayout->addRow("所属线路：", lineBox);
        stationLayout->addRow("站点名：", stationNameEdit);
        stationLayout->addRow("横坐标 x：", xEdit);
        stationLayout->addRow("纵坐标 y：", yEdit);
        stationLayout->addRow("连接前站：", prevStationBox);
        stationLayout->addRow(addStationBtn);
        layout->addWidget(stationGroup);

        connect(lineBox, &QComboBox::currentTextChanged, this, [this]() {
            refreshPrevStationBox();
        });

        connect(addStationBtn, &QPushButton::clicked, this, [this]() {
            QString line = lineBox->currentText();
            QString station = stationNameEdit->text().trimmed();
            bool okX = false, okY = false;
            double x = xEdit->text().toDouble(&okX);
            double y = yEdit->text().toDouble(&okY);

            if (line.isEmpty()) {
                QMessageBox::warning(this, "提示", "请先添加或选择一条线路。");
                return;
            }
            if (station.isEmpty()) {
                QMessageBox::warning(this, "提示", "站点名不能为空。");
                return;
            }
            if (!okX || !okY) {
                QMessageBox::warning(this, "提示", "坐标必须是数字。");
                return;
            }

            QString prev;
            if (prevStationBox->currentIndex() > 0) {
                prev = prevStationBox->currentText();
            }

            graph.addStation(line, station, QPointF(x, y), prev);
            stationNameEdit->clear();
            xEdit->clear();
            yEdit->clear();
            refreshAllComboBoxes();
            drawNetwork();
        });

        resultText = new QTextEdit;
        resultText->setReadOnly(true);
        resultText->setPlaceholderText("查询结果会显示在这里……");
        layout->addWidget(new QLabel("换乘指南："));
        layout->addWidget(resultText, 1);

        QPushButton *resetBtn = new QPushButton("清除路径高亮");
        layout->addWidget(resetBtn);
        connect(resetBtn, &QPushButton::clicked, this, [this]() {
            drawNetwork();
        });

        return panel;
    }

    void addChain(const QString &line, const QColor &color, const QVector<StationPoint> &items) {
        graph.addLine(line, color);
        QString prev;
        for (const StationPoint &item : items) {
            graph.addStation(line, item.first, item.second, prev);
            prev = item.first;
        }
    }

    void buildShanghaiMetro() {
        addChain("1号线", QColor(231, 59, 65), {
                                                   {"莘庄", {558, 1138}}, {"外环路", {588, 1109}}, {"莲花路", {614, 1082}},
                                                   {"锦江乐园", {641, 1056}}, {"上海南站", {681, 1021}}, {"漕宝路", {712, 952}},
                                                   {"上海体育馆", {733, 880}}, {"徐家汇", {780, 827}}, {"衡山路", {815, 797}},
                                                   {"常熟路", {854, 772}}, {"陕西南路", {906, 777}}, {"一大会址·黄陂南路", {973, 735}},
                                                   {"人民广场", {1015, 683}}, {"新闸路", {992, 652}}, {"汉中路", {963, 622}},
                                                   {"上海火车站", {967, 561}}, {"中山北路", {967, 510}}, {"延长路", {968, 477}},
                                                   {"上海马戏城", {966, 433}}, {"汶水路", {967, 391}}, {"彭浦新村", {967, 360}},
                                                   {"共康路", {968, 330}}, {"通河新村", {968, 300}}, {"呼兰路", {965, 264}},
                                                   {"共富新村", {968, 221}}, {"宝安公路", {967, 189}}, {"友谊西路", {967, 158}},
                                                   {"富锦路", {968, 127}}
                                               });

        addChain("2号线", QColor(155, 204, 46), {
                                                    {"蟠祥路·国家会计学院", {249, 749}}, {"国家会展中心(2号线)", {313, 749}}, {"虹桥火车站", {370, 744}},
                                                    {"虹桥2号航站楼", {429, 744}}, {"淞虹路", {519, 683}}, {"北新泾", {561, 683}},
                                                    {"威宁路", {603, 683}}, {"娄山关路", {644, 683}}, {"中山公园", {716, 683}},
                                                    {"江苏路", {775, 683}}, {"静安寺", {855, 683}}, {"南京西路", {911, 683}},
                                                    {"人民广场", {1015, 683}}, {"南京东路", {1075, 683}}, {"陆家嘴", {1132, 694}},
                                                    {"浦东南路(2号线)", {1165, 727}}, {"世纪大道", {1220, 781}}, {"上海科技馆", {1264, 863}},
                                                    {"世纪公园", {1264, 897}}, {"龙阳路", {1296, 955}}, {"张江高科", {1405, 952}},
                                                    {"金科路", {1465, 952}}, {"广兰路", {1534, 948}}, {"唐镇", {1639, 953}},
                                                    {"创新中路", {1673, 988}}, {"华夏东路", {1673, 1036}}, {"川沙", {1700, 1102}},
                                                    {"凌空路", {1750, 1102}}, {"远东大道", {1805, 1102}}, {"海天三路", {1800, 1174}},
                                                    {"浦东1号2号航站楼", {1743, 1219}}
                                                });

        addChain("3号线", QColor(251, 213, 53), {
                                                    {"上海南站", {681, 1021}}, {"石龙路", {740, 994}}, {"龙漕路", {781, 952}},
                                                    {"漕溪路", {760, 924}}, {"宜山路", {716, 827}}, {"虹桥路", {716, 782}},
                                                    {"延安西路", {718, 732}}, {"中山公园", {716, 683}}, {"金沙江路", {716, 598}},
                                                    {"曹杨路", {774, 561}}, {"镇坪路", {855, 561}}, {"中潭路", {911, 561}},
                                                    {"上海火车站", {967, 561}}, {"宝山路", {1048, 561}}, {"东宝兴路", {1076, 525}},
                                                    {"虹口足球场", {1075, 483}}, {"赤峰路", {1091, 432}}, {"大柏树", {1120, 404}},
                                                    {"江湾镇", {1151, 368}}, {"殷高西路", {1183, 341}}, {"长江南路", {1207, 315}},
                                                    {"淞发路", {1218, 278}}, {"张华浜", {1219, 243}}, {"淞滨路", {1219, 208}},
                                                    {"水产路", {1219, 175}}, {"宝杨路", {1219, 139}}, {"友谊路", {1219, 105}},
                                                    {"铁力路", {1182, 77}}, {"江杨北路", {1100, 77}}
                                                });

        addChain("4号线", QColor(124, 96, 175), {
                                                    {"宜山路", {716, 827}}, {"上海体育馆", {733, 880}}, {"上海体育场", {785, 884}},
                                                    {"东安路", {855, 880}}, {"大木桥路", {906, 880}}, {"鲁班路", {958, 880}},
                                                    {"西藏南路", {1019, 880}}, {"南浦大桥", {1068, 880}}, {"塘桥", {1126, 880}},
                                                    {"蓝村路", {1176, 880}}, {"向城路", {1220, 835}}, {"世纪大道", {1220, 781}},
                                                    {"浦东大道", {1219, 683}}, {"杨树浦路", {1219, 635}}, {"大连路", {1220, 591}},
                                                    {"临平路", {1182, 564}}, {"海伦路", {1116, 564}}, {"宝山路", {1048, 561}},
                                                    {"上海火车站", {967, 561}}, {"中潭路", {911, 561}}, {"镇坪路", {855, 561}},
                                                    {"曹杨路", {774, 561}}, {"金沙江路", {716, 598}}, {"中山公园", {716, 683}},
                                                    {"延安西路", {718, 732}}, {"虹桥路", {716, 782}}, {"宜山路", {716, 827}}
                                                });

        addChain("5号线", QColor(163, 118, 201), {
                                                     {"莘庄", {558, 1138}}, {"春申路", {543, 1186}}, {"银都路", {538, 1216}},
                                                     {"颛桥", {543, 1245}}, {"北桥", {543, 1273}}, {"剑川路", {543, 1301}},
                                                     {"东川路", {543, 1328}}, {"江川路", {543, 1380}}, {"西渡", {543, 1411}},
                                                     {"萧塘", {543, 1441}}, {"奉浦大道", {543, 1471}}, {"环城东路", {587, 1490}},
                                                     {"望园路", {639, 1490}}, {"金海湖", {697, 1490}}, {"奉贤新城", {748, 1490}}
                                                 });

        addChain("5号线", QColor(163, 118, 201), {
                                                     {"东川路", {543, 1328}}, {"金平路", {499, 1343}}, {"华宁路", {461, 1342}},
                                                     {"文井路", {423, 1342}}, {"闵行开发区", {386, 1342}}
                                                 });

        addChain("6号线", QColor(251, 49, 152), {
                                                    {"港城路", {1474, 227}}, {"外高桥保税区北", {1535, 260}}, {"航津路", {1534, 290}},
                                                    {"外高桥保税区南", {1535, 322}}, {"洲海路", {1498, 424}}, {"五洲大道", {1499, 462}},
                                                    {"东靖路", {1500, 520}}, {"巨峰路", {1499, 563}}, {"五莲路", {1499, 605}},
                                                    {"博兴路", {1474, 646}}, {"金桥路", {1445, 674}}, {"云山路", {1412, 707}},
                                                    {"德平路", {1369, 736}}, {"北洋泾路", {1334, 736}}, {"民生路", {1296, 736}},
                                                    {"源深体育中心", {1253, 748}}, {"世纪大道", {1220, 781}}, {"浦电路", {1176, 835}},
                                                    {"蓝村路", {1176, 880}}, {"上海儿童医学中心", {1166, 927}}, {"临沂新村", {1139, 955}},
                                                    {"高科西路", {1118, 1002}}, {"东明路", {1118, 1051}}, {"高青路", {1118, 1084}},
                                                    {"华夏西路", {1085, 1102}}, {"上南路", {1052, 1102}}, {"灵岩南路", {1017, 1102}},
                                                    {"东方体育中心", {963, 1101}}
                                                });

        addChain("7号线", QColor(252, 119, 74), {
                                                    {"美兰湖", {713, 70}}, {"罗南新村", {714, 105}}, {"潘广路", {714, 138}},
                                                    {"刘行", {714, 173}}, {"顾村公园", {713, 206}}, {"祁华路", {713, 238}},
                                                    {"上海大学", {766, 264}}, {"南陈路", {813, 265}}, {"上大路", {855, 287}},
                                                    {"场中路", {855, 315}}, {"大场镇", {855, 343}}, {"行知路", {854, 373}},
                                                    {"大华三路", {855, 401}}, {"新村路", {852, 434}}, {"岚皋路", {855, 501}},
                                                    {"镇坪路", {855, 561}}, {"长寿路", {854, 598}}, {"昌平路", {855, 640}},
                                                    {"静安寺", {855, 683}}, {"常熟路", {854, 772}}, {"肇嘉浜路", {855, 828}},
                                                    {"东安路", {855, 880}}, {"龙华中路", {880, 930}}, {"后滩", {917, 965}},
                                                    {"长清路", {971, 1001}}, {"耀华路", {1019, 1002}}, {"云台路", {1085, 1002}},
                                                    {"高科西路", {1118, 1002}}, {"杨高南路", {1156, 1002}}, {"锦绣路", {1191, 1002}},
                                                    {"芳华路", {1244, 973}}, {"龙阳路", {1296, 955}}, {"花木路", {1364, 896}}
                                                });

        addChain("8号线", QColor(60, 158, 234), {
                                                    {"市光路", {1364, 322}}, {"嫩江路", {1361, 368}}, {"翔殷路", {1364, 424}},
                                                    {"黄兴公园", {1364, 462}}, {"延吉中路", {1364, 501}}, {"黄兴路", {1332, 525}},
                                                    {"江浦路", {1295, 524}}, {"鞍山新村", {1240, 525}}, {"四平路", {1183, 497}},
                                                    {"曲阳路", {1139, 484}}, {"虹口足球场", {1075, 483}}, {"西藏北路", {1019, 502}},
                                                    {"中兴路", {1019, 539}}, {"曲阜路", {1019, 622}}, {"人民广场", {1015, 683}},
                                                    {"大世界", {1019, 735}}, {"老西门", {1019, 781}}, {"陆家浜路", {1019, 828}},
                                                    {"西藏南路", {1019, 880}}, {"中华艺术宫", {1019, 939}}, {"耀华路", {1019, 1002}},
                                                    {"成山路", {998, 1052}}, {"杨思", {978, 1072}}, {"东方体育中心", {963, 1101}},
                                                    {"凌兆新村", {963, 1137}}, {"芦恒路", {968, 1186}}, {"浦江镇", {968, 1220}},
                                                    {"江月路", {968, 1253}}, {"联航路", {968, 1289}}, {"沈杜公路", {967, 1322}}
                                                });

        addChain("9号线", QColor(137, 204, 232), {
                                                     {"上海松江站", {169, 1323}}, {"醉白池", {169, 1263}}, {"松江体育中心", {169, 1204}},
                                                     {"松江新城", {169, 1140}}, {"松江大学城", {169, 1091}}, {"洞泾", {172, 1002}},
                                                     {"佘山", {169, 937}}, {"泗泾", {213, 880}}, {"九亭", {286, 880}},
                                                     {"中春路", {378, 880}}, {"七宝", {418, 880}}, {"星中路", {466, 880}},
                                                     {"合川路", {530, 880}}, {"漕河泾开发区", {594, 881}}, {"桂林路", {642, 863}},
                                                     {"宜山路", {716, 827}}, {"徐家汇", {780, 827}}, {"肇嘉浜路", {855, 828}},
                                                     {"嘉善路", {905, 828}}, {"打浦桥", {938, 828}}, {"马当路", {967, 828}},
                                                     {"陆家浜路", {1019, 828}}, {"小南门", {1078, 806}}, {"商城路", {1160, 779}},
                                                     {"世纪大道", {1220, 781}}, {"杨高中路", {1296, 781}}, {"芳甸路", {1369, 781}},
                                                     {"蓝天路", {1440, 736}}, {"台儿庄路", {1473, 705}}, {"金桥", {1534, 680}},
                                                     {"金吉路", {1573, 680}}, {"金海路", {1612, 683}}, {"顾唐路", {1659, 683}},
                                                     {"民雷路", {1700, 683}}, {"曹路", {1741, 683}}
                                                 });

        addChain("10号线", QColor(199, 171, 209), {
                                                      {"虹桥火车站", {370, 744}}, {"虹桥2号航站楼", {429, 744}}, {"虹桥1号航站楼", {483, 739}},
                                                      {"上海动物园", {519, 740}}, {"龙溪路", {561, 739}}, {"水城路", {602, 739}},
                                                      {"伊犁路", {642, 747}}, {"宋园路", {666, 771}}, {"虹桥路", {716, 782}},
                                                      {"交通大学", {774, 781}}, {"上海图书馆", {812, 782}}, {"陕西南路", {906, 777}},
                                                      {"一大会址·新天地", {967, 781}}, {"老西门", {1019, 781}}, {"豫园", {1075, 734}},
                                                      {"南京东路", {1075, 683}}, {"天潼路", {1075, 622}}, {"四川北路", {1091, 589}},
                                                      {"海伦路", {1116, 564}}, {"邮电新村", {1149, 532}}, {"四平路", {1183, 497}},
                                                      {"同济大学", {1212, 468}}, {"国权路", {1241, 438}}, {"五角场", {1261, 419}},
                                                      {"江湾体育场", {1279, 401}}, {"三门路", {1294, 369}}, {"殷高东路", {1296, 323}},
                                                      {"新江湾城", {1296, 290}}, {"国帆路", {1296, 259}}, {"双江路", {1363, 222}},
                                                      {"高桥西", {1401, 223}}, {"高桥", {1438, 222}}, {"港城路", {1474, 227}},
                                                      {"基隆路", {1512, 223}}
                                                  });

        addChain("10号线", QColor(199, 171, 209), {
                                                      {"龙溪路", {561, 739}}, {"龙柏新村", {536, 796}}, {"紫藤路", {519, 828}},
                                                      {"航中路", {483, 827}}
                                                  });

        addChain("11号线", QColor(145, 62, 77), {
                                                    {"嘉定北", {464, 128}}, {"嘉定西", {463, 161}}, {"白银路", {463, 194}},
                                                    {"嘉定新城", {476, 236}}, {"马陆", {499, 259}}, {"陈翔公路", {522, 282}},
                                                    {"南翔", {556, 313}}, {"桃浦新村", {573, 333}}, {"武威路", {596, 357}},
                                                    {"祁连山路", {619, 379}}, {"李子园", {643, 404}}, {"上海西站", {673, 433}},
                                                    {"真如", {723, 484}}, {"枫桥路", {754, 514}}, {"曹杨路", {774, 561}},
                                                    {"隆德路", {775, 597}}, {"江苏路", {775, 683}}, {"交通大学", {774, 781}},
                                                    {"徐家汇", {780, 827}}, {"上海游泳馆", {776, 900}}, {"龙华", {814, 952}},
                                                    {"云锦路", {842, 980}}, {"龙耀路", {867, 1005}}, {"东方体育中心", {963, 1101}},
                                                    {"三林", {1052, 1155}}, {"三林东", {1109, 1155}}, {"浦三路", {1167, 1155}},
                                                    {"康恒路", {1224, 1155}}, {"御桥", {1296, 1155}}, {"罗山路", {1354, 1155}},
                                                    {"秀沿路", {1363, 1189}}, {"康新公路", {1406, 1238}}, {"迪士尼", {1524, 1234}}
                                                });

        addChain("11号线", QColor(145, 62, 77), {
                                                    {"嘉定新城", {476, 236}}, {"上海赛车场", {394, 222}}, {"昌吉东路", {345, 257}},
                                                    {"上海汽车城", {302, 276}}, {"安亭", {262, 277}}, {"兆丰路", {222, 276}},
                                                    {"光明路", {182, 277}}, {"花桥", {131, 277}}
                                                });

        addChain("12号线", QColor(39, 147, 88), {
                                                    {"七莘路", {418, 1003}}, {"虹莘路", {466, 1002}}, {"顾戴路", {504, 988}},
                                                    {"东兰路", {527, 965}}, {"虹梅路", {564, 952}}, {"虹漕路", {603, 952}},
                                                    {"桂林公园", {642, 952}}, {"漕宝路", {712, 952}}, {"龙漕路", {781, 952}},
                                                    {"龙华", {814, 952}}, {"龙华中路", {880, 930}}, {"大木桥路", {906, 880}},
                                                    {"嘉善路", {905, 828}}, {"陕西南路", {906, 777}}, {"南京西路", {911, 683}},
                                                    {"汉中路", {963, 622}}, {"曲阜路", {1019, 622}}, {"天潼路", {1075, 622}},
                                                    {"国际客运中心", {1117, 622}}, {"提篮桥", {1160, 625}}, {"大连路", {1220, 591}},
                                                    {"江浦公园", {1296, 563}}, {"宁国路", {1332, 563}}, {"隆昌路", {1364, 564}},
                                                    {"爱国路", {1397, 563}}, {"复兴岛", {1430, 563}}, {"东陆路", {1462, 564}},
                                                    {"巨峰路", {1499, 563}}, {"杨高北路", {1534, 560}}, {"金京路", {1576, 563}},
                                                    {"申江路", {1613, 609}}, {"金海路", {1612, 683}}
                                                });

        addChain("13号线", QColor(252, 151, 192), {
                                                      {"金运路", {413, 597}}, {"金沙江西路", {477, 598}}, {"丰庄", {519, 598}},
                                                      {"祁连山南路", {562, 597}}, {"真北路", {603, 597}}, {"大渡河路", {642, 597}},
                                                      {"金沙江路", {716, 598}}, {"隆德路", {775, 597}}, {"武宁路", {815, 597}},
                                                      {"长寿路", {854, 598}}, {"江宁路", {911, 598}}, {"汉中路", {963, 622}},
                                                      {"自然博物馆", {937, 649}}, {"南京西路", {911, 683}}, {"淮海中路", {934, 739}},
                                                      {"一大会址·新天地", {967, 781}}, {"马当路", {967, 828}}, {"世博会博物馆", {967, 899}},
                                                      {"世博大道", {970, 958}}, {"长清路", {971, 1001}}, {"成山路", {998, 1052}},
                                                      {"东明路", {1118, 1051}}, {"华鹏路", {1156, 1051}}, {"下南路", {1191, 1052}},
                                                      {"北蔡", {1224, 1052}}, {"陈春路", {1259, 1052}}, {"莲溪路", {1296, 1051}},
                                                      {"华夏中路", {1354, 1051}}, {"中科路", {1428, 1051}}, {"学林路", {1496, 1052}},
                                                      {"张江路", {1573, 1052}}
                                                  });

        addChain("14号线", QColor(145, 145, 70), {
                                                     {"封浜", {407, 389}}, {"乐秀路", {446, 423}}, {"临洮路", {467, 448}},
                                                     {"嘉怡路", {490, 471}}, {"定边路", {519, 484}}, {"真新新村", {561, 485}},
                                                     {"真光路", {603, 483}}, {"铜川路", {642, 484}}, {"真如", {723, 484}},
                                                     {"中宁路", {786, 502}}, {"曹杨路", {774, 561}}, {"武宁路", {815, 597}},
                                                     {"武定路", {822, 650}}, {"静安寺", {855, 683}}, {"一大会址·黄陂南路", {973, 735}},
                                                     {"大世界", {1019, 735}}, {"豫园", {1075, 734}}, {"陆家嘴", {1132, 694}},
                                                     {"浦东南路(14号线)", {1168, 683}}, {"浦东大道", {1219, 683}}, {"源深路", {1258, 683}},
                                                     {"昌邑路", {1296, 683}}, {"歇浦路", {1334, 683}}, {"云山路", {1412, 707}},
                                                     {"蓝天路", {1440, 736}}, {"黄杨路", {1469, 766}}, {"云顺路", {1499, 781}},
                                                     {"浦东足球场", {1535, 777}}, {"金粤路", {1573, 782}}, {"桂桥路", {1612, 781}}
                                                 });

        addChain("15号线", QColor(187, 168, 136), {
                                                      {"顾村公园", {713, 206}}, {"锦秋路", {731, 238}}, {"丰翔路", {731, 286}},
                                                      {"南大路", {731, 319}}, {"祁安路", {731, 351}}, {"古浪路", {724, 385}},
                                                      {"武威东路", {702, 406}}, {"上海西站", {673, 433}}, {"铜川路", {642, 484}},
                                                      {"梅岭北路", {642, 541}}, {"大渡河路", {642, 597}}, {"长风公园", {642, 640}},
                                                      {"娄山关路", {644, 683}}, {"红宝石路", {642, 766}}, {"姚虹路", {642, 796}},
                                                      {"吴中路", {642, 826}}, {"桂林路", {642, 863}}, {"桂林公园", {642, 952}},
                                                      {"上海南站", {681, 1021}}, {"华东理工大学", {711, 1079}}, {"罗秀路", {710, 1112}},
                                                      {"朱梅路", {710, 1141}}, {"景洪路", {714, 1173}}, {"虹梅南路", {689, 1203}},
                                                      {"景西路", {661, 1204}}, {"曙建路", {639, 1217}}, {"双柏路", {640, 1245}},
                                                      {"元江路", {640, 1272}}, {"永德路", {640, 1300}}, {"紫竹高新区", {636, 1342}}
                                                  });

        addChain("16号线", QColor(128, 216, 213), {
                                                      {"龙阳路", {1296, 955}}, {"华夏中路", {1354, 1051}}, {"罗山路", {1354, 1155}},
                                                      {"周浦东", {1354, 1222}}, {"鹤沙航城", {1354, 1289}}, {"航头东", {1354, 1355}},
                                                      {"新场", {1406, 1424}}, {"野生动物园", {1464, 1424}}, {"惠南", {1522, 1424}},
                                                      {"惠南东", {1608, 1454}}, {"书院", {1674, 1486}}, {"临港大道", {1745, 1487}},
                                                      {"滴水湖", {1828, 1483}}
                                                  });

        addChain("17号线", QColor(199, 135, 136), {
                                                      {"虹桥火车站", {370, 744}}, {"国家会展中心(17号线)", {313, 739}}, {"蟠龙路", {266, 718}},
                                                      {"徐盈路", {246, 697}}, {"徐泾北城", {214, 683}}, {"嘉松中路", {183, 683}},
                                                      {"赵巷", {152, 683}}, {"汇金路", {121, 683}}, {"青浦新城", {87, 680}},
                                                      {"漕盈路", {63, 708}}, {"淀山湖大道", {62, 751}}, {"朱家角", {62, 793}},
                                                      {"东方绿舟", {62, 837}}, {"西岑", {67, 879}}
                                                  });

        addChain("18号线", QColor(219, 173, 120), {
                                                      {"康文路", {911, 265}}, {"呼兰路", {965, 264}}, {"爱辉路", {1013, 264}},
                                                      {"长江西路", {1134, 265}}, {"通南路", {1179, 286}}, {"长江南路", {1207, 315}},
                                                      {"殷高路", {1218, 343}}, {"上海财经大学", {1216, 368}}, {"复旦大学", {1219, 405}},
                                                      {"国权路", {1241, 438}}, {"抚顺路", {1276, 474}}, {"江浦路", {1295, 524}},
                                                      {"江浦公园", {1296, 563}}, {"平凉路", {1296, 603}}, {"丹阳路", {1296, 635}},
                                                      {"昌邑路", {1296, 683}}, {"民生路", {1296, 736}}, {"杨高中路", {1296, 781}},
                                                      {"迎春路", {1296, 836}}, {"龙阳路", {1296, 955}}, {"芳芯路", {1296, 1001}},
                                                      {"北中路", {1296, 1026}}, {"莲溪路", {1296, 1051}}, {"御桥", {1296, 1155}},
                                                      {"康桥", {1295, 1190}}, {"周浦", {1296, 1223}}, {"繁荣路", {1296, 1256}},
                                                      {"沈梅路", {1296, 1289}}, {"鹤涛路", {1296, 1322}}, {"下沙", {1295, 1355}},
                                                      {"航头", {1296, 1388}}
                                                  });

        addChain("浦江线", QColor(179, 179, 197), {
                                                      {"沈杜公路", {967, 1322}}, {"三鲁公路", {1000, 1343}}, {"闵瑞路", {1038, 1356}},
                                                      {"浦航路", {1037, 1380}}, {"东城一路", {1038, 1407}}, {"汇臻路", {1000, 1424}}
                                                  });

        addChain("市域机场线", QColor(104, 108, 114), {
                                                          {"虹桥2号航站楼(市域)", {406, 721}}, {"中春路", {378, 880}}, {"景洪路", {714, 1173}},
                                                          {"三林南", {924, 1173}}, {"康桥东", {1446, 1173}}, {"上海国际旅游度假区", {1525, 1173}},
                                                          {"浦东1号2号航站楼(市域)", {1728, 1219}}
                                                      });

        addChain("磁浮线", QColor(111, 188, 184), {
                                                      {"龙阳路", {1296, 955}}, {"浦东1号2号航站楼", {1743, 1219}}
                                                  });

        addChain("金山铁路", QColor(104, 116, 131), {
                                                        {"上海南站", {681, 1021}}, {"莘庄", {558, 1138}}, {"春申", {353, 1174}},
                                                        {"新桥", {320, 1208}}, {"车墩", {304, 1280}}, {"叶榭", {304, 1330}},
                                                        {"亭林", {305, 1380}}, {"金山园区", {304, 1430}}, {"金山卫", {194, 1480}}
                                                    });
    }




    void refreshAllComboBoxes() {
        QString oldStart = startBox ? startBox->currentText() : QString();
        QString oldTarget = targetBox ? targetBox->currentText() : QString();
        QString oldLine = lineBox ? lineBox->currentText() : QString();

        QStringList stations = graph.allStations();
        startBox->clear();
        targetBox->clear();
        startBox->addItems(stations);
        targetBox->addItems(stations);

        if (!oldStart.isEmpty()) startBox->setCurrentText(oldStart);
        if (!oldTarget.isEmpty()) targetBox->setCurrentText(oldTarget);
        if (targetBox->count() > 1 && startBox->currentText() == targetBox->currentText()) {
            targetBox->setCurrentIndex(1);
        }

        lineBox->clear();
        lineBox->addItems(graph.allLines());
        if (!oldLine.isEmpty()) lineBox->setCurrentText(oldLine);

        refreshPrevStationBox();
    }

    void refreshPrevStationBox() {
        if (!prevStationBox || !lineBox) return;
        QString line = lineBox->currentText();
        prevStationBox->clear();
        prevStationBox->addItem("不连接前站");
        prevStationBox->addItems(graph.stationsOfLine(line));
    }

    QString makeGuideText(const RouteResult &route, const QString &start, const QString &target) const {
        if (!route.error.isEmpty()) return route.error;

        QString text;
        text += QString("起点站：%1\n").arg(start);
        text += QString("终点站：%1\n").arg(target);

        if (route.states.size() <= 1) {
            text += "起点和终点相同，无需乘车。\n";
            return text;
        }

        text += QString("经过站数：%1\n").arg(route.stops);
        text += QString("换乘次数：%1\n\n").arg(route.transfers);

        QStringList transferStations;
        QString currentLine = route.states[1].line;
        QString segmentStart = route.states[0].station;

        text += "乘车方案：\n";
        text += QString("1. 在【%1】乘坐【%2】。\n").arg(segmentStart, currentLine);

        int step = 2;
        for (int i = 2; i < route.states.size(); ++i) {
            QString nextLine = route.states[i].line;
            if (nextLine != currentLine) {
                QString transferStation = route.states[i - 1].station;
                transferStations << transferStation;
                text += QString("%1. 乘坐【%2】：%3 → %4。\n")
                            .arg(step++)
                            .arg(currentLine, segmentStart, transferStation);
                text += QString("%1. 在【%2】换乘【%3】。\n")
                            .arg(step++)
                            .arg(transferStation, nextLine);
                segmentStart = transferStation;
                currentLine = nextLine;
            }
        }

        text += QString("%1. 乘坐【%2】：%3 → %4。\n")
                    .arg(step++)
                    .arg(currentLine, segmentStart, target);
        text += QString("%1. 到达终点【%2】。\n\n").arg(step++).arg(target);

        text += "换乘站：";
        text += transferStations.isEmpty() ? "无\n" : transferStations.join("、") + "\n";

        QStringList pathStations;
        for (const RouteState &s : route.states) {
            pathStations << s.station;
        }
        text += "完整路径：\n" + pathStations.join(" → ") + "\n";

        return text;
    }

    void drawNetwork(const QVector<RouteState> &route = QVector<RouteState>()) {
        scene->clear();
        scene->setSceneRect(0, 0, 1900, 1500);

        const auto &stations = graph.getStations();
        const auto &adj = graph.getAdj();

        QGraphicsRectItem *bg = scene->addRect(scene->sceneRect(), Qt::NoPen, QBrush(QColor(34, 39, 46)));
        bg->setZValue(-10);

        drawTitleAndLegend();

        QSet<QString> drawnEdges;
        for (auto it = adj.begin(); it != adj.end(); ++it) {
            QString from = it.key();
            if (!stations.contains(from)) continue;

            for (const Edge &e : it.value()) {
                if (!stations.contains(e.to)) continue;
                QString a = from < e.to ? from : e.to;
                QString b = from < e.to ? e.to : from;
                QString edgeKey = e.line + "|" + a + "|" + b;
                if (drawnEdges.contains(edgeKey)) continue;
                drawnEdges.insert(edgeKey);

                QPointF p1 = stations[from].pos;
                QPointF p2 = stations[e.to].pos;
                QPen pen(graph.colorOfLine(e.line), 5);
                pen.setCapStyle(Qt::RoundCap);
                scene->addLine(QLineF(p1, p2), pen)->setZValue(0);
            }
        }

        QSet<QString> routeStationSet;
        for (const RouteState &s : route) {
            routeStationSet.insert(s.station);
        }
        for (int i = 1; i < route.size(); ++i) {
            QString a = route[i - 1].station;
            QString b = route[i].station;
            if (!stations.contains(a) || !stations.contains(b)) continue;
            QPen pen(QColor(255, 255, 255), 11);
            pen.setCapStyle(Qt::RoundCap);
            QGraphicsLineItem *whiteLine = scene->addLine(QLineF(stations[a].pos, stations[b].pos), pen);
            whiteLine->setZValue(1);

            QPen redPen(QColor(255, 70, 70), 7);
            redPen.setCapStyle(Qt::RoundCap);
            QGraphicsLineItem *redLine = scene->addLine(QLineF(stations[a].pos, stations[b].pos), redPen);
            redLine->setZValue(2);
        }

        for (auto it = stations.begin(); it != stations.end(); ++it) {
            const Station &s = it.value();
            bool isTransfer = s.lines.size() > 1;
            bool inRoute = routeStationSet.contains(s.name);
            qreal r = isTransfer ? 7 : 5;

            QColor fill = inRoute ? QColor(255, 245, 110)
                                  : (isTransfer ? QColor(255, 255, 255) : QColor(230, 230, 230));
            QPen pen(inRoute ? QColor(255, 255, 255) : QColor(15, 15, 15), inRoute ? 2 : 1);
            QGraphicsEllipseItem *dot = scene->addEllipse(s.pos.x() - r, s.pos.y() - r, 2 * r, 2 * r, pen, QBrush(fill));
            dot->setZValue(3);

            QGraphicsTextItem *label = scene->addText(s.name);
            QFont font("Microsoft YaHei", 8);
            if (isTransfer) font.setBold(true);
            label->setFont(font);
            label->setDefaultTextColor(Qt::white);   // 按要求：站名改成白色
            label->setPos(s.pos.x() + 8, s.pos.y() - 13);
            label->setZValue(5);

            QRectF labelRect = label->boundingRect().translated(label->pos()).adjusted(-2, -1, 2, 1);
            QGraphicsRectItem *labelBg = scene->addRect(labelRect, Qt::NoPen, QBrush(QColor(0, 0, 0, 110)));
            labelBg->setZValue(4);
        }
    }

    void drawTitleAndLegend() {
        QGraphicsTextItem *title = scene->addText("SHANGHAI 上海地铁网络图");
        QFont titleFont("Microsoft YaHei", 22);
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setDefaultTextColor(QColor(80, 170, 255));
        title->setPos(25, 20);
        title->setZValue(10);

        QGraphicsTextItem *tip = scene->addText("提示：白色站名为程序绘制标签；红色高亮为查询路径。坐标为课程设计模拟布局。");
        tip->setFont(QFont("Microsoft YaHei", 10));
        tip->setDefaultTextColor(QColor(220, 220, 220));
        tip->setPos(28, 58);
        tip->setZValue(10);

        qreal x = 1750;
        qreal y = 80;
        QGraphicsTextItem *legendTitle = scene->addText("线路 LINES");
        QFont legendFont("Microsoft YaHei", 12);
        legendFont.setBold(true);
        legendTitle->setFont(legendFont);
        legendTitle->setDefaultTextColor(Qt::white);
        legendTitle->setPos(x, y - 35);
        legendTitle->setZValue(10);

        int i = 0;
        for (const QString &line : graph.allLines()) {
            qreal yy = y + i * 26;
            QPen pen(graph.colorOfLine(line), 6);
            pen.setCapStyle(Qt::RoundCap);
            scene->addLine(x, yy + 8, x + 48, yy + 8, pen)->setZValue(10);
            QGraphicsTextItem *text = scene->addText(line);
            text->setFont(QFont("Microsoft YaHei", 9));
            text->setDefaultTextColor(Qt::white);
            text->setPos(x + 58, yy - 3);
            text->setZValue(10);
            ++i;
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
