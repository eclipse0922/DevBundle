#include "jrcscore.h"
#include <QThread>
#include <strstream>
#include "MeshColor.h"
#include "densecrf3d.h"
namespace JRCS{

void JRCSBase::reset_iteration()
{
    iter_count_ = 0;
}

void JRCSBase::input(
        const MatPtrLst& vv,
        const MatPtrLst& vn,
        const CMatPtrLst& vc,
        const LCMatPtrLst& vl,
        bool verbose
        )
{
    vvs_ptrlst_ = vv;
    vns_ptrlst_ = vn;
    vcs_ptrlst_ = vc;
    vls_ptrlst_ = vl;
    verbose_ = verbose;
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
    xtv_ = *xv_ptr_;
    xtn_ = *xn_ptr_;
    xtc_ = *xc_ptr_;
    std::cerr<<"obj_prob:"<<std::endl;
    std::cerr<<obj_prob_<<std::endl;
    int k = xv_ptr_->n_cols;
    std::cerr<<k<<std::endl;
    int r_k = k;
    float* pxv = (float*)xtv_.memptr();
    float* pxn = (float*)xtn_.memptr();
    uint8_t* pxc = (uint8_t*)xtc_.memptr();
    int N = obj_prob_.size();
    obj_pos_ = arma::fmat(3,N,arma::fill::zeros);
    arma::frowvec z = arma::linspace<arma::frowvec>(float(-N/2),float(N/2),N);
    obj_pos_.row(2) = z;
    max_obj_radius_ = 0.0;
    std::cerr<<"obj_pos_:"<<std::endl;
    std::cerr<<obj_pos_<<std::endl;
    obj_label_ = arma::uvec(xv_ptr_->n_cols,arma::fill::zeros);
    arma::uword* pxl = (arma::uword*)obj_label_.memptr();
    for(int obj_idx = 0 ; obj_idx < obj_prob_.size() ; ++ obj_idx )
    {
        int obj_size = int(float(k)*float(obj_prob_(obj_idx)));
        obj_size = std::max(9,obj_size);
        obj_size = std::min(r_k,obj_size);
        objv_ptrlst_.emplace_back(new arma::fmat(pxv,3,obj_size,false,true));
        objn_ptrlst_.emplace_back(new arma::fmat(pxn,3,obj_size,false,true));
        objc_ptrlst_.emplace_back(new arma::Mat<uint8_t>(pxc,3,obj_size,false,true));
        arma::uvec lbl(pxl,obj_size,false,true);
        lbl.fill(arma::uword(obj_idx+1));
        pxv += 3*obj_size;
        pxn += 3*obj_size;
        pxc += 3*obj_size;
        pxl += obj_size;
        r_k -= obj_size;
        arma::fvec pos = obj_pos_.col(obj_idx);
        reset_obj_vn(0.5,pos,(*objv_ptrlst_.back()),(*objn_ptrlst_.back()));
        reset_obj_c((*objc_ptrlst_.back()));
    }
    *xv_ptr_ = xtv_;
    *xn_ptr_ = xtn_;
    *xc_ptr_ = xtc_;
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
    if( radius > max_obj_radius_ ) max_obj_radius_ = radius;
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
    oc.row(2).fill(255);
}

void JRCSBase::reset_rt()
{
    if(vvs_ptrlst_.empty())
    {
        throw std::logic_error("JRCSBase::reset_rt: no input");
    }
    rt_lst_.resize(vvs_ptrlst_.size());
    TsLst::iterator iter;
    for(iter=rt_lst_.begin();iter!=rt_lst_.end();++iter)
    {
        Ts& rt = *iter ;
        rt.resize(obj_num_);
        #pragma omp for
        for( int i = 0 ; i < obj_num_ ; ++i )
        {
            arma::fmat R(rt[i].R,3,3,false,true);
            arma::fvec t(rt[i].t,3,false,true);
            R.fill(arma::fill::eye);
            arma::fvec randw(2*obj_num_,arma::fill::randn);
            randw = arma::normalise(randw);
            arma::fvec randt(3,arma::fill::randn);
            arma::fmat pos0 = obj_pos_.each_col() + randt*max_obj_radius_;
            arma::fmat pos1 = obj_pos_.each_col() - randt*max_obj_radius_;
            arma::fmat pos_base = arma::join_rows(pos0,pos1);
            t = pos_base*randw;
        }
    }
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
    xv_sum_.fill(0.0);
    xn_sum_.fill(0.0);
    xc_sum_.fill(0.0);
    var_sum.fill(0.0);
    alpha_sum.fill(0.0);
    alpha_sumij.fill(0.0);


    //reset transformed latent center
    if(verbose_)std::cerr<<"reset transformed latent color"<<std::endl;
    xtc_ = *xc_ptr_;

    computeCompatibility(mu_);

    for(int idx=0;idx<vvs_ptrlst_.size();++idx)
    {
        if(verbose_)std::cerr<<"reset transformed latent center"<<std::endl;
        xtv_ = *xv_ptr_;
        xtn_ = *xn_ptr_;

        arma::fmat& vv_ = *vvs_ptrlst_[idx];
        arma::fmat& vn_ = *vns_ptrlst_[idx];
        arma::Mat<uint8_t>& vc_ = *vcs_ptrlst_[idx];
        arma::fmat& alpha = *alpha_ptrlst_[idx];
        Ts& rt = rt_lst_[idx];

        if(verbose_)std::cerr<<"step-E"<<std::endl;
        if(verbose_)std::cerr<<"transform object"<<std::endl;
        #pragma omp for
        for(int o = 0 ; o < obj_num_ ; ++o )
        {
            arma::fmat R(rt[o].R,3,3,false,true);
            arma::fvec t(rt[o].t,3,false,true);
            arma::fmat& objv = *objv_ptrlst_[o];
            arma::fmat& objn = *objn_ptrlst_[o];
            objv = R*objv;
            objv.each_col() += t;
            objn = R*objn;
        }
        if(verbose_)std::cerr<<"calculate alpha"<<std::endl;
        #pragma omp for
        for(int r = 0 ; r < alpha.n_rows ; ++r )
        {
            arma::fmat tmpm = xtv_.each_col() - vv_.col(r);
            //project distance to normal direction
//            tmpm %= xtn_;
            alpha.row(r) = arma::sum(arma::square(tmpm));
//            alpha.row(r) = arma::square(arma::sum(tmpm));
        }

        alpha.each_row() %= (-0.5*x_invvar_);
        alpha = arma::exp(alpha);
        alpha.each_row() %= arma::pow(x_invvar_,1.5);
        alpha.each_row() %= x_p_;

        #pragma omp for
        for(int o = 0 ; o < obj_num_ ; ++o )
        {
            arma::uvec oidx = arma::find(obj_label_==(o+1));
            alpha.cols(oidx) *= obj_prob_(o);
        }

        arma::fvec alpha_rowsum = ( 1.0 + beta_ ) * arma::sum(alpha,1);
        alpha.each_col() /= alpha_rowsum;
        alpha_rowsum = arma::sum(alpha,1);

        //smoothing alpha
        if(verbose_)std::cerr<<"smoothing alpha"<<std::endl;

        arma::fmat vc = arma::conv_to<arma::fmat>::from(vc_);
        DenseCRF3D crf(vv_,vn_,vc,xv_ptr_->n_cols);
        arma::mat unary = arma::conv_to<arma::mat>::from(alpha);

        crf.setUnaryEnergy(unary.t());
        arma::fvec sxyz = { 0.1 , 0.1 , 0.1 } ;
        crf.addPairwiseGaussian(sxyz,new MatrixCompatibility(mu_));
        arma::fvec srgb = { 25 , 25 , 25 };
        arma::fvec snxyz = { 0.01 , 0.01, 0.01 };
        crf.addPairwiseBilateral(sxyz,snxyz,srgb,new MatrixCompatibility(mu_));

        if(verbose_)std::cerr<<"start smoothing"<<std::endl;
        arma::mat Q = crf.startInference();
        arma::mat t1,t2;
        std::cerr<<"kl = "<<crf.klDivergence(Q)<<std::endl;
        for( int it=0; it<3; it++ ) {
            crf.stepInference( Q, t1, t2 );
            std::cerr<<"kl = "<<crf.klDivergence(Q)<<std::endl;
        }
        alpha = arma::conv_to<arma::fmat>::from(Q.t());

        //update RT
        //#1 calculate weighted point cloud
        if(verbose_)std::cerr<<"calculating the weighted point cloud"<<std::endl;
        arma::fmat& wv = *wvs_ptrlst_[idx];
        arma::fmat& wn = *wns_ptrlst_[idx];
        arma::fmat wc = arma::conv_to<arma::fmat>::from(*wcs_ptrlst_[idx]);
        arma::frowvec alpha_colsum = arma::sum( alpha );

        wv = vv_*alpha;
        wn = vn_*alpha;
        wc = arma::conv_to<arma::fmat>::from(vc_)*alpha;

        #pragma omp parallel for
        for(int c=0;c<alpha.n_cols;++c)
        {
            if( 0 != alpha_colsum(c) )
            {
                wv.col(c) /= alpha_colsum(c);
                wn.col(c) /= alpha_colsum(c);
                wc.col(c) /= alpha_colsum(c);
            }
        }

        wn = arma::normalise( wn );
        *wcs_ptrlst_[idx] = arma::conv_to<arma::Mat<uint8_t>>::from(wc);

        if(verbose_)std::cerr<<"calculating R & t"<<std::endl;
        #pragma omp parallel for
        for(int o = 0 ; o < obj_num_ ; ++o )
        {
            arma::fmat A;
            arma::fmat U,V;
            arma::fvec s;
            arma::fmat C(3,3,arma::fill::eye);
            arma::fmat R(rt[o].R,3,3,false,true);
            arma::fvec t(rt[o].t,3,false,true);
            arma::fmat dR;
            arma::fvec dt;
            arma::fmat objv = *objv_ptrlst_[o];
            arma::uvec oidx = arma::find(obj_label_==(o+1));
            arma::fmat v = wv.cols(oidx);
            arma::fmat cv = v.each_col() - arma::mean(v,1);
            objv.each_col() -= arma::mean(objv,1);
            A = cv*objv.t();

            if(arma::svd(U,s,V,A,"std"))
            {
                C(2,2) = arma::det( U * V.t() )>=0 ? 1.0 : -1.0;
                dR = U*C*(V.t());
                dt = arma::mean( v - dR*(*objv_ptrlst_[o]),1);
            }

            //updating objv
            *objv_ptrlst_[o] = dR*(*objv_ptrlst_[o]);
            (*objv_ptrlst_[o]).each_col() += dt;
            *objn_ptrlst_[o] = dR*(*objn_ptrlst_[o]);

            //updating R T
            R = dR*R;
            t = dR*t + dt;

            //accumulate for updating X
            arma::fmat tv = v.each_col() - t;
            xv_sum_.cols(oidx) +=  R.i()*tv;
            xn_sum_.cols(oidx) +=  R.i()*wn.cols(oidx);
            xc_sum_.cols(oidx) += wc.cols(oidx);
        }
        //update var
        alpha_sum += alpha_colsum;
        arma::fmat alpha_2(alpha.n_rows,alpha.n_cols);
        #pragma omp parallel for
        for(int r=0;r<alpha_2.n_rows;++r)
        {
            alpha_2.row(r) = arma::sum(arma::square(xtv_.each_col() - vv_.col(r)));
        }
        arma::frowvec tmpvar = arma::sum(alpha_2%alpha);
        var_sum += tmpvar;
        alpha_sumij += alpha_colsum;
    }

    if(verbose_)std::cerr<<"Updating X:"<<std::endl;
    float N =  vvs_ptrlst_.size();
    *xv_ptr_ = xv_sum_ / N;
    if(!(*xv_ptr_).is_finite()){
        throw std::logic_error("infinite xv");
    }

    //fix the x center position
    #pragma omp for
    for(int o = 0 ; o < obj_num_ ; ++o )
    {
        arma::uvec oidx = arma::find(obj_label_==(o+1));
        arma::fmat newxv = xv_ptr_->cols(oidx);
        arma::fvec t =  obj_pos_.col(o) - arma::mean(newxv,1);
        xv_ptr_->cols(oidx) = newxv.each_col() + t;
    }
    *xn_ptr_ = xn_sum_ / N;
    *xn_ptr_ = arma::normalise(*xn_ptr_);
    *xc_ptr_ = arma::conv_to<arma::Mat<uint8_t>>::from( xc_sum_ / N );

    x_invvar_ = ( (3.0*alpha_sum ) / ( var_sum + 1e-6 ) );//restore reciprocal fractions of variation
    float mu = arma::accu(alpha_sumij);
    mu *= ( 1.0 + beta_ );
    x_p_ = alpha_sumij;
    if( mu != 0)x_p_ /= mu;
}

void JRCSBase::computeCompatibility(arma::mat& mu)
{
    if(verbose_)std::cerr<<"computeCompatibility"<<std::endl;
    mu = arma::mat(xv_ptr_->n_cols,xv_ptr_->n_cols,arma::fill::zeros);
    #pragma omp for
    for(int o = 0 ; o < obj_num_ ; ++o )
    {
        arma::uvec oidx = arma::find(obj_label_==(o+1));
        arma::fmat xv = xv_ptr_->cols(oidx);
        arma::mat m;
        m = mu(oidx,oidx);
        for(int i=0 ; i < xv.n_cols ; ++i )
        {
            for(int j=i+1 ; j < xv.n_cols ; ++j )
            {
                arma::fvec dx = xv.col(i) - xv.col(j);
                m(i,j) = -0.1*std::exp(-arma::sum(arma::square( dx / 0.05 )));
            }
        }
        mu(oidx,oidx) = m ;
    }
}

void JRCSBase::reset_alpha()
{
    if(verbose_)std::cerr<<"allocating alpha"<<std::endl;
    arma::fmat& xv_ = *xv_ptr_;
    alpha_ptrlst_.clear();
    int idx=0;
    while(alpha_ptrlst_.size()<vvs_ptrlst_.size())
    {
        alpha_ptrlst_.emplace_back(new arma::fmat(vvs_ptrlst_[idx]->n_cols,xv_.n_cols));
        ++idx;
    }
    if(verbose_)std::cerr<<"done allocating alpha"<<std::endl;
}

void JRCSBase::reset_prob()
{
    if(verbose_)std::cerr<<"initializing probability"<<std::endl;
    arma::fvec maxAllXYZ,minAllXYZ;
    MatPtrLst::iterator vviter;
    for( vviter = vvs_ptrlst_.begin() ; vviter != vvs_ptrlst_.end() ; ++vviter )
    {
        arma::fmat&v = **vviter;
        arma::fvec maxVXYZ = arma::max(v,1);
        arma::fvec minVXYZ = arma::min(v,1);
        if(vviter==vvs_ptrlst_.begin())
        {
            maxAllXYZ = maxVXYZ;
            minAllXYZ = minVXYZ;
        }else{
            maxAllXYZ = arma::max(maxVXYZ,maxAllXYZ);
            minAllXYZ = arma::min(minVXYZ,minAllXYZ);
        }
    }

    float maxvar = arma::accu(arma::square(maxAllXYZ-minAllXYZ));
    x_invvar_ = arma::frowvec(xv_ptr_->n_cols);
    x_invvar_.fill(1.0/maxvar);

    xv_sum_ = arma::fmat(xv_ptr_->n_rows,xv_ptr_->n_cols,arma::fill::zeros);
    xn_sum_ = arma::fmat(xn_ptr_->n_rows,xn_ptr_->n_cols,arma::fill::zeros);
    xc_sum_ = arma::fmat(xc_ptr_->n_rows,xc_ptr_->n_cols,arma::fill::zeros);

    x_p_ = arma::frowvec(xv_ptr_->n_cols);
    x_p_.fill(1.0/float(xv_ptr_->n_cols));

    var_sum = arma::frowvec(xv_ptr_->n_cols,arma::fill::zeros);
    alpha_sum = arma::frowvec(xv_ptr_->n_cols,arma::fill::zeros);
    alpha_sumij = arma::frowvec(xv_ptr_->n_cols,arma::fill::zeros);

    beta_ = 0.01;
    if(verbose_)std::cerr<<"done probability"<<std::endl;
}

void JRCSBase::update_color_label()
{
    if(verbose_)std::cerr<<"updating color label"<<std::endl;
    for(int idx=0;idx<vvs_ptrlst_.size();++idx)
    {
        arma::fmat& alpha = *alpha_ptrlst_[idx];
        arma::fmat obj_p(alpha.n_rows,obj_num_);
        arma::Col<uint32_t>& vl = *vls_ptrlst_[idx];
        #pragma omp for
        for(int o = 0 ; o < obj_num_ ; ++o )
        {
            arma::uvec oidx = arma::find(obj_label_==(o+1));
            arma::fmat sub_alpha = alpha.cols(oidx);
            obj_p.col(o) = arma::sum(sub_alpha,1);
        }
        arma::uvec label(alpha.n_rows);
        #pragma omp for
        for(int r = 0 ; r < obj_p.n_rows ; ++r )
        {
            arma::uword l;
            arma::frowvec point_prob = obj_p.row(r);
            point_prob.max(l);
            label(r) = l+1;
        }
        ColorArray::colorfromlabel((uint32_t*)vl.memptr(),vl.size(),label);
    }
}

bool JRCSBase::isEnd()
{
    if(iter_count_>=50)return true;
    return false;
}
}