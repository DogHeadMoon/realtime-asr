#include "segment/segment.h"
#include <iostream>

Acts::Acts(){
    reset();
}

bool Acts::lookback_sil(){
    int end = acts_.size() -1;
    int hyp_st = acts_.size() - sil_threshold_;
    int st = hyp_st > 0 ? hyp_st : 0;
    for(int i=st;i<end;i++){
        if(acts_[i]){
            return false;
        }
    }
    return true;
}

int Acts::check_window(int window_idx){
    const int window_ = 15;
    const int min_act_ = 5;

    int st = window_idx * window_;
    int nacts = 0;
    for(int i=0;i<window_;i++){
        if(acts_[st+i] == 1){
            nacts++;
        }
    }

    if(nacts >= min_act_){
        return 1;
    }
    else if(nacts == 0){
        return 2;
    }
    else{
        return 0;
    }
}

void Acts::end(){
    if(seg_sts_.size() == seg_ends_.size() + 1) {
        int nframes = acts_.size();
        int st = seg_sts_.back();
        int dur = nframes - st;
        std::cout<<"final dur : "<<dur<<std::endl;
        std::cout<<"final act_percent : "<<act_percent(st, nframes-1)<<std::endl;
        if(dur > 100 && act_percent(st, nframes-1) > 0.5){
            seg_ends_.emplace_back(nframes-1);
        }
        //seg_ends_.emplace_back(nframes-1);
        //std::cout<<"seg ends emplace_back frame idx : "<<nframes-1<<std::endl;
        last_seg_idx_ = nframes-1;
    }
}

void Acts::add_act(int act, bool last){
    acts_.emplace_back(act);
    
    int nframes = acts_.size();
    if(nframes % window_ == 0){
        int window_idx = (nframes-1)/window_;
        int status = check_window(window_idx);
        wind_stats_.emplace_back(status);
        int nwind = wind_stats_.size();

        if(status == 1){
            if(seg_sts_.size() == seg_ends_.size()){
                seg_sts_.emplace_back(nframes-1);
                std::cout<<"seg sts emplace_back frame idx : "<<nframes-1<<std::endl;
            }
        }
        else if(status == 2){
            std::cout<<"status == 2 "<<nframes-1<<std::endl;
            //if((seg_sts_.size() == seg_ends_.size() + 1) || last) {
            int st = 0;
            if(seg_sts_.size()>0){
                st = seg_sts_.back();
            }
            int dur = nframes-1 - st;
            if( ((seg_sts_.size() == seg_ends_.size() + 1) && dur>200)   || last ) {
                seg_ends_.emplace_back(nframes-1);
                std::cout<<"seg ends emplace_back frame idx : "<<nframes-1<<std::endl;
                last_seg_idx_ = nframes-1;
            }
        }
        else{
        }
    }
}

int Acts::get_seg_size(){
    return segs_.size();
}

vector<pair<int, int> > Acts::get_segs(int start_idx){
    const int nsamples_per_frame = 160;
    int n = segs_.size();
    vector<pair<int, int> > rt;
    if(n>start_idx){
        for(int i=start_idx;i<n;i++){
            rt.emplace_back(std::make_pair(segs_[i].first * nsamples_per_frame, 
                segs_[i].second * nsamples_per_frame));
        }
    }
    return rt;
}

void Acts::reset(){
    acts_.clear();
    seg_sts_.clear();
    seg_ends_.clear();
    wind_stats_.clear();
    last_seg_idx_ = 0;
}

float Acts::act_percent(int st, int end){
    float size = end - st;
    int n = 0;
    for(int i=st;i<end;i++){
        n += acts_[i];
    }
    return n/size;
}
