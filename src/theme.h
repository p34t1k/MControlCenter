// Neon-dark theme for Mcenter (matches the Kali / Dr460nized desktop). Applied as a stylesheet on the main window only.
#ifndef THEME_H
#define THEME_H
static const char *kThemeQss = R"QSS(
QMainWindow, QTabWidget, QTabBar, QWidget#centralwidget, QWidget#dashboard, QWidget#overviewTab, QWidget#batteryTab, QWidget#fanControlTab, QWidget#keyboardTab, QWidget#aboutTab, QWidget#debugTab, QTabWidget QStackedWidget > QWidget { background: #0e0f11; }
QWidget { color: #e9e6df; font-family: "Rajdhani"; font-size: 16px; font-weight: 500; }
QTabWidget::pane { border: none; background: transparent; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; color: #8b8f96; padding: 9px 18px; margin: 4px 3px 6px 3px; border-radius: 0px; border: 1px solid transparent; font-weight: 600; letter-spacing: 1px; }
QTabBar::tab:hover { color: #e9e6df; background: #15171a; }
QTabBar::tab:selected { color: #ff4b1f; background: #15171a; border: 1px solid #ff4b1f; }
QLabel#dashTitle { font-size: 26px; font-weight: 800; letter-spacing: 2px; color: #ffffff; }
QLabel#dashSub { color: #8b8f96; font-size: 12px; letter-spacing: 1px; }
QLabel#section { color: #ff4b1f; font-size: 11px; font-weight: 700; letter-spacing: 2px; padding-top: 2px; }
QLabel#chip { background: #15171a; border: 1px solid #2c2f34; border-radius: 0px; padding: 5px 14px; font-weight: 700; font-size: 11px; letter-spacing: 1px; }
QLabel#chip[tone="ok"] { color: #9ccd5a; border-color: #4d6b2c; }
QLabel#chip[tone="warn"] { color: #ffb020; border-color: #7a5410; }
QFrame#card { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #1a1c20, stop:1 #121315); border: 1px solid #2c2f34; border-radius: 0px; }
QLabel#statKey { color: #8b8f96; background: transparent; }
QLabel#statVal { color: #ffffff; font-weight: 700; background: transparent; }
QGroupBox { border: 1px solid #2c2f34; border-radius: 0px; margin-top: 14px; padding: 14px 10px 10px 10px; background: #15171a; font-weight: 700; }
QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 8px; color: #ff4b1f; letter-spacing: 1px; }
QPushButton { background: #15171a; border: 1px solid #2c2f34; border-radius: 0px; padding: 7px 16px; font-weight: 600; }
QPushButton:hover { border-color: #ff4b1f; color: #ff4b1f; }
QPushButton:pressed { background: #ff4b1f; color: #0e0f11; }
QPushButton:disabled { color: #5b5f66; border-color: #23262b; }
QCheckBox, QRadioButton { spacing: 9px; background: transparent; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 5px; border: 1px solid #3d4147; background: #15171a; }
QCheckBox::indicator:hover, QRadioButton::indicator:hover { border-color: #ff4b1f; }
QCheckBox::indicator:checked { background: #ff4b1f; border-color: #ff4b1f; }
QRadioButton::indicator { width: 18px; height: 18px; border-radius: 0px; border: 1px solid #3d4147; background: #15171a; }
QRadioButton::indicator:checked { background: qradialgradient(cx:.5,cy:.5,radius:.5,fx:.5,fy:.5,stop:0 #ff4b1f,stop:.55 #ff4b1f,stop:.6 #15171a,stop:1 #15171a); border-color: #ff4b1f; }
QCheckBox:disabled, QRadioButton:disabled { color: #5b5f66; }
QSlider::groove:horizontal { height: 6px; background: #23262b; border-radius: 3px; }
QSlider::sub-page:horizontal { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #ff4b1f, stop:1 #ffb08a); border-radius: 3px; }
QSlider::handle:horizontal { width: 16px; height: 16px; margin: -6px 0; border-radius: 0px; background: #ffffff; border: 2px solid #ff4b1f; }
QSlider::groove:vertical { width: 6px; background: #23262b; border-radius: 3px; }
QSlider::add-page:vertical { background: qlineargradient(x1:0,y1:1,x2:0,y2:0, stop:0 #ff4b1f, stop:1 #ffb08a); border-radius: 3px; }
QSlider::sub-page:vertical { background: #23262b; border-radius: 3px; }
QSlider::handle:vertical { height: 16px; width: 16px; margin: 0 -6px; border-radius: 0px; background: #ffffff; border: 2px solid #ff4b1f; }
QSpinBox { min-width: 74px; }
QTabBar::tab:focus { outline: none; }
QSpinBox, QComboBox, QLineEdit, QDoubleSpinBox { background: #15171a; border: 1px solid #2c2f34; border-radius: 0px; padding: 5px 9px; selection-background-color: #ff4b1f; selection-color: #0e0f11; }
QSpinBox:focus, QComboBox:focus, QLineEdit:focus { border-color: #ff4b1f; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background: #15171a; border: 1px solid #ff4b1f; selection-background-color: #ff4b1f; selection-color: #0e0f11; outline: none; }
QScrollBar:vertical { background: transparent; width: 10px; } QScrollBar::handle:vertical { background: #2c2f34; border-radius: 5px; min-height: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; } QScrollBar:horizontal { background: transparent; height: 10px; } QScrollBar::handle:horizontal { background: #2c2f34; border-radius: 5px; }
QToolTip { background: #15171a; color: #e9e6df; border: 1px solid #ff4b1f; padding: 5px; }
QMenu { background: #15171a; border: 1px solid #2c2f34; padding: 6px; } QMenu::item { padding: 7px 22px; border-radius: 6px; } QMenu::item:selected { background: #ff4b1f; color: #0e0f11; }
)QSS";
#endif
