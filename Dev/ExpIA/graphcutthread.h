#ifndef GRAPHCUTTHREAD_H
#define GRAPHCUTTHREAD_H
#include <QThread>
#include "objectmodel.h"
#include "graphcut.h"
#include "common.h"
#include <typeinfo>
#include <QTime>
class GraphCutThread:public QThread
{
    Q_OBJECT
public:
    GraphCutThread(
            MeshBundle<DefaultMesh>::PtrList&inputmesh,
            std::vector<ObjModel::Ptr>& inputobj,
            std::vector<arma::uvec>& outputlabels,
            QObject* parent=0
            ):QThread(parent),meshes_(inputmesh),objects_(inputobj),outputs_(outputlabels)
    {
        setObjectName("GraphCutThread");
        current_frame_ = 0;
    }

public:
    bool configure(Config::Ptr config);
signals:
    void message(QString,int);
    void sendMatch(int,MeshBundle<DefaultMesh>::Ptr);
protected:
    void run(void);
    void showMatch(size_t,DefaultMesh&);
    void showData(size_t);
    void showSmooth();

    bool prepareDataTerm();
    void prepareDataForLabel(
            uint32_t l,
            VoxelGraph<DefaultMesh>& graph,
            DefaultMesh& obj,
            arma::fvec &dist_score,
            arma::fvec &norm_score,
            arma::fvec &color_score
            );
    void prepareDataForUnknown();
    void normalizeData();

    std::shared_ptr<double> data_;
    std::shared_ptr<DataCost> current_data_;

    bool prepareSmoothTerm(Segmentation::GraphCut&);
    std::shared_ptr<SmoothnessCost> current_smooth_;
    static MRF::CostVal fnCost(int pix1,int pix2,MRF::Label i,MRF::Label j);
    static arma::sp_mat smooth_;

    bool prepareNeighbors(Segmentation::GraphCut&);

    uint32_t current_frame_;
    uint32_t label_number_;
    uint32_t pix_number_;

private:
    MeshBundle<DefaultMesh>::PtrList& meshes_;
    std::vector<ObjModel::Ptr>& objects_;
    std::vector<arma::uvec>& outputs_;
    Config::Ptr config_;
    QTime timer;
};

#endif // GRAPHCUTTHREAD_H
