#include <core.h>

class SrcFilter : public epf::Filter {
  public:
    SrcFilter();
    ~SrcFilter();

  protected:
    int32_t _job();
    int32_t _open();
    int32_t _close();
    int32_t _set();
    int32_t _reset();
    int32_t _start();
    int32_t _stop();

  private:
    epf::QueueWriter *writer_;  //  QueueWriter writer(q);
    int32_t counter_init{10};
    int32_t my_counter_{0};
    double my_counter2_{0};
    float my_array_[4]{1, 2, 3, 4};

    epf::DataNode *counter_n_;
    epf::DataNode *counter_n2_;
    epf::DataNode *img_encd_size_;

    epf::DataNode *array_;
    epf::DataNode *double_matrix_;
};
