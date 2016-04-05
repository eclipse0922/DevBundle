#include "jrcscore.h"
namespace JRCS{

void JRCSBase::reset_iteration()
{
    iter_count_ = 0;
    obj_num_ = 3;
}

void JRCSBase::input(
        const MatPtrLst& vv,
        const MatPtrLst& vn,
        const CMatPtrLst& vc,
        const LCMatPtrLst& vl
        )
{
    vvs_ptrlst_ = vv;
    vns_ptrlst_ = vn;
    vcs_ptrlst_ = vc;
    vls_ptrlst_ = vl;
}

void JRCSBase::resetw(
        const MatPtrLst& wv,
        const MatPtrLst& wn,
        const CMatPtrLst& wc
        )
{
    wvs_ptrlst_ = wv;
    wns_ptrlst_ = wn;
    wcs_ptrlst_ = wc;
    int k = wvs_ptrlst_.front()->n_cols;

    MatPtrLst::iterator wviter = wvs_ptrlst_.begin();
    MatPtrLst::iterator vviter = vvs_ptrlst_.begin();
    MatPtrLst::iterator wniter = wns_ptrlst_.begin();
    MatPtrLst::iterator vniter = vns_ptrlst_.begin();
    CMatPtrLst::iterator wciter = wcs_ptrlst_.begin();
    CMatPtrLst::iterator vciter = vcs_ptrlst_.begin();
    for( int idx = 0; idx < wvs_ptrlst_.size() ; ++ idx )
    {
        int range = ( *vviter )->n_cols - 1;
        arma::uvec index = arma::randi<arma::uvec>( k, arma::distr_param( 0 , range ) );

        **wviter = (**vviter).cols(index);
        **wniter = (**vniter).cols(index);
        **wciter = (**vciter).cols(index);

        ++wviter;
        ++vviter;
        ++wniter;
        ++vniter;
        ++wciter;
        ++vciter;
    }
}

void JRCSBase::reset_objw(const std::vector<float>& pvec)
{
    std::cerr<<"JRCSBase::reset_objw"<<std::endl;
    obj_num_ = pvec.size();
    obj_prob_= arma::fmat(pvec);
    obj_prob_ = arma::sort(obj_prob_);
}

void JRCSBase::initx(
        const MatPtr& xv,
        const MatPtr& xn,
        const CMatPtr& xc
        )
{
    xv_ptr_ = xv;
    xn_ptr_ = xn;
    xc_ptr_ = xc;
    std::cerr<<"obj_prob:"<<std::endl;
    std::cerr<<obj_prob_<<std::endl;
    int k = xv_ptr_->n_cols;
    std::cerr<<k<<std::endl;
    int r_k = k;
    float* pxv = (float*)xv_ptr_->memptr();
    float* pxn = (float*)xn_ptr_->memptr();
    uint8_t* pxc = (uint8_t*)xc_ptr_->memptr();
    int N = obj_prob_.size();
    obj_pos_ = arma::fmat(3,N,arma::fill::zeros);
    arma::frowvec z = arma::linspace<arma::frowvec>(float(-N),float(N),N);
    obj_pos_.row(2) = z;
    std::cerr<<"obj_pos_:"<<std::endl;
    std::cerr<<obj_pos_<<std::endl;
    for(int obj_idx = 0 ; obj_idx < obj_prob_.size() ; ++ obj_idx )
    {
        int obj_size = int(float(k)*float(obj_prob_(obj_idx)));
        obj_size = std::max(9,obj_size);
        obj_size = std::min(r_k,obj_size);
        objv_ptrlst_.emplace_back(new arma::fmat(pxv,3,obj_size,false,true));
        objn_ptrlst_.emplace_back(new arma::fmat(pxn,3,obj_size,false,true));
        objc_ptrlst_.emplace_back(new arma::Mat<uint8_t>(pxc,3,obj_size,false,true));
        pxv += 3*obj_size;
        pxn += 3*obj_size;
        pxc += 3*obj_size;
        r_k -= obj_size;
        arma::fvec pos = obj_pos_.col(obj_idx);
        reset_obj_vn(0.5,pos,(*objv_ptrlst_.back()),(*objn_ptrlst_.back()));
        reset_obj_c((*objc_ptrlst_.back()));
    }
}

void JRCSBase::reset_obj_vn(
        float radius,
        arma::fvec& pos,
        arma::fmat& ov,
        arma::fmat& on
        )
{
    rand_sphere(ov);
    on = ov;
    ov *= radius;
    ov.each_col() += pos;
}

void JRCSBase::rand_sphere(
        arma::fmat& ov
        )
{
    int k = ov.n_cols;
    arma::frowvec az = arma::randu<arma::frowvec>(k);
    arma::frowvec el = arma::randu<arma::frowvec>(k);
    az*=2*M_PI;
    el*=2*M_PI;
    ov.row(0) = arma::cos(az)%arma::cos(el);
    ov.row(1) = arma::sin(el);
    ov.row(2) = arma::sin(az)%arma::cos(el);
}

void JRCSBase::reset_obj_c(
        arma::Mat<uint8_t>& oc
        )
{
    oc.fill(0);
    oc.row(1).fill(255);
}

int JRCSBase::evaluate_k()
{
    if(vvs_ptrlst_.empty())
    {
        throw std::logic_error("need input v before evaluate_k");
    }
    MatPtrLst::iterator iter;
    arma::fvec k_lst(vvs_ptrlst_.size());
    int idx = 0;
    for(iter=vvs_ptrlst_.begin();iter!=vvs_ptrlst_.end();++iter)
    {
        k_lst(idx) = (*iter)->n_cols;
        ++idx;
    }
    return ( arma::median(k_lst) / 2 + 5 ) ;//half of the median size but at least five;
}

void JRCSBase::computeOnce()
{
    stepE();
    stepM();
}

void JRCSBase::stepE()
{
    ;
}

void JRCSBase::stepM()
{
    ;
}

bool JRCSBase::isEnd()
{
    if(iter_count_>=2)return true;
    return false;
}
}
