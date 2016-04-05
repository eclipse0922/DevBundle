#ifndef JRCSVIEW_H
#define JRCSVIEW_H

#include <QFrame>
#include "common.h"
#include "objectmodel.h"
#include "MeshListViewerWidget.h"
#include "jrcsthread.h"
namespace Ui {
class JRCSView;
}

class JRCSView : public QFrame
{
    Q_OBJECT
public:
    typedef std::vector<MeshBundle<DefaultMesh>::Ptr> MeshList;
    typedef std::vector<arma::uvec> LabelList;
    typedef std::vector<ObjModel::Ptr> ModelList;
    typedef JRCSThread::MatPtr MatPtr;
    typedef JRCSThread::CMatPtr CMatPtr;
    typedef JRCSThread::MatPtrLst MatPtrLst;
    typedef JRCSThread::CMatPtrLst CMatPtrLst;
    typedef JRCSThread::LCMatPtrLst LCMatPtrLst;
    explicit JRCSView(
            MeshList& inputs,
            LabelList& labels,
            ModelList& objects,
            QWidget *parent = 0
            );
    ~JRCSView();
    bool configure(Config::Ptr);
    bool init(Config::Ptr);
    void start();
signals:
    void message(QString,int);
    void closeInMdi(QWidget*);
protected slots:
    void passMessage(QString,int);
    void finished();
protected:
    void input( JRCSThread* jrcs_worker_ );
    bool allocate_x( JRCSThread* jrcs_worker_ );
    void move_worker_to_thread( JRCSThread* jrcs_worker );
private:
    Ui::JRCSView *ui;
    MeshListViewerWidget* geo_view_;
    QThread* jrcs_thread_;
    MeshList& inputs_;
    LabelList& labels_;
    ModelList& objects_;
};

#endif // JRCSVIEW_H
