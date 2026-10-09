#include "UI/mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include "Utils/applogger.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("MonitoringSystem"));
    a.setOrganizationName(QStringLiteral("MonitoringSystem"));
    a.setWindowIcon(QIcon(QStringLiteral(":/assets/monitoring-app.svg")));
    a.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));
    a.setStyleSheet(QStringLiteral(R"(
        QWidget { color: #1E293B; font-family: "Microsoft YaHei UI"; }
        QMainWindow, QDialog { background: #F1F5F9; }
        QPushButton { background: #FFFFFF; color: #334155; border: 1px solid #CBD5E1; border-radius: 7px; padding: 7px 14px; min-height: 22px; font-weight: 600; }
        QPushButton:hover { background: #EFF6FF; border-color: #60A5FA; color: #1D4ED8; }
        QPushButton:pressed { background: #DBEAFE; }
        QPushButton:disabled { color: #94A3B8; background: #F1F5F9; }
        #addDeviceBtn, #startAllBtn, #tcpServerBtn { background: #2563EB; border-color: #2563EB; color: #FFFFFF; }
        #addDeviceBtn:hover, #startAllBtn:hover, #tcpServerBtn:hover { background: #1D4ED8; border-color: #1D4ED8; color: #FFFFFF; }
        #stopAllBtn { background: #FFF7F7; border-color: #FECACA; color: #B91C1C; }
        #stopAllBtn:hover { background: #FEE2E2; border-color: #F87171; }
        QTableWidget, QPlainTextEdit, QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox { background: #FFFFFF; border: 1px solid #D8E0EA; border-radius: 6px; selection-background-color: #BFDBFE; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #60A5FA; }
        QComboBox, QLineEdit { min-height: 30px; padding: 2px 8px; }
        QTableWidget { gridline-color: #E2E8F0; alternate-background-color: #F8FAFC; }
        QTableWidget::item { padding: 5px; }
        QTableWidget::item:selected { background: #DBEAFE; color: #1E3A8A; }
        QSpinBox, QDoubleSpinBox { min-height: 30px; min-width: 132px; padding-left: 28px; padding-right: 28px; }
        QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-origin: border; subcontrol-position: left center; width: 26px; border: 0; border-right: 1px solid #1D4ED8; border-top-left-radius: 5px; border-bottom-left-radius: 5px; background: #2563EB; }
        QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-origin: border; subcontrol-position: right center; width: 26px; border: 0; border-left: 1px solid #1D4ED8; border-top-right-radius: 5px; border-bottom-right-radius: 5px; background: #2563EB; }
        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover, QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover { background: #1D4ED8; }
        QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed, QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed { background: #1E40AF; }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow { width: 8px; height: 8px; }
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow { width: 8px; height: 8px; }
        QHeaderView::section { background: #EAF0F7; color: #334155; border: 0; border-bottom: 1px solid #CBD5E1; padding: 7px; font-weight: 600; }
        QGroupBox { background: #FFFFFF; border: 1px solid #DCE4EE; border-radius: 9px; margin-top: 12px; padding: 10px; font-weight: 600; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QStatusBar { background: #FFFFFF; border-top: 1px solid #DCE4EE; }
        QTabWidget::pane { background: #FFFFFF; border: 1px solid #DCE4EE; border-radius: 7px; top: -1px; }
        QTabBar::tab { background: #EAF0F7; color: #475569; border: 1px solid #DCE4EE; padding: 8px 16px; margin-right: 3px; border-top-left-radius: 6px; border-top-right-radius: 6px; }
        QTabBar::tab:selected { background: #FFFFFF; color: #1D4ED8; font-weight: 700; border-bottom-color: #FFFFFF; }
        QTabBar::tab:hover:!selected { background: #F8FAFC; }
        QScrollBar:vertical { background: #F1F5F9; width: 11px; margin: 2px; border-radius: 5px; }
        QScrollBar::handle:vertical { background: #CBD5E1; min-height: 28px; border-radius: 5px; }
        QScrollBar::handle:vertical:hover { background: #94A3B8; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )"));
    AppLogger::install();
    MainWindow w;
    w.show();
    return a.exec();
}
