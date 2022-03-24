#ifndef SEGMENT_H_
#define SEGMENT_H_

#include <vector>
using namespace std;

class Acts{
public:
    Acts();
    void add_act(int act, bool last);
    bool lookback_sil();
    int get_seg_size();
    vector<pair<int, int> > get_segs(int start_idx);
    int check_window(int window_idx);
    void reset();
    void end();
    float act_percent(int st, int end);

    vector<int> seg_sts_;
    vector<int> seg_ends_;

private:
    vector<int> acts_;
    vector<pair<int, int>> segs_;
    const int sil_threshold_=100;
    const int max_seg_len_=2000;
    
    int last_act_;
    int act_acum_;
    int noact_acum_;
    int begin_;

    const int window_ = 15;
    vector<int> wind_stats_;
    int last_seg_idx_;
};

#endif