#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMenu>
#include <QToolBar>
#include <QMainWindow>

#include "svnrev.h"

const QString VERSION("1.0");

class QLabel;
class SnafuWidget;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
  MainWindow(QWidget * parent=0);
  ~MainWindow();

  SnafuWidget * snafu() { return mSW; }

  virtual void closeEvent( QCloseEvent * );

private:
  SnafuWidget * mSW;

  QLabel * mCounterLabel;
  QMenu * mFileMenu, * mViewMenu, * mHelpMenu;
  QToolBar * HostToolBar;
};

#endif // MAIN_WINDOW_H

