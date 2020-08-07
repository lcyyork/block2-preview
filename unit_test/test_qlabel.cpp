
#include "block2.hpp"
#include "gtest/gtest.h"

using namespace block2;

class TestQ : public ::testing::Test {
  protected:
    static const int n_tests = 10000000;
    void SetUp() override { Random::rand_seed(0); }
    void TearDown() override {}
};

template <typename S> struct QZLabel {
    const static int nmin, nmax, tsmin, tsmax;
    int n, twos, pg;
    QZLabel() {
        n = Random::rand_int(nmin, nmax + 1);
        twos = Random::rand_int(tsmin, tsmax + 1);
        pg = Random::rand_int(0, 8);
        if ((n & 1) != (twos & 1))
            twos = (twos & (~1)) | (n & 1);
    }
    QZLabel(int n, int twos, int pg) : n(n), twos(twos), pg(pg) {}
    bool in_range() const {
        return n >= nmin && n <= nmax && twos >= tsmin && twos <= tsmax;
    }
    int multi() const { return 1; }
    int fermion() const { return n & 1; }
    QZLabel<S> operator-() const { return QZLabel<S>(-n, -twos, pg); }
    QZLabel<S> operator+(QZLabel<S> other) const {
        return QZLabel<S>(n + other.n, twos + other.twos, pg ^ other.pg);
    }
    QZLabel<S> operator-(QZLabel<S> other) const { return *this + (-other); }
    static void check() {
        QZLabel<S> qq, qq2, qq3;
        S q(qq.n, qq.twos, qq.pg);
        // getter
        EXPECT_EQ(q.n(), qq.n);
        EXPECT_EQ(q.twos(), qq.twos);
        EXPECT_EQ(q.pg(), qq.pg);
        EXPECT_EQ(q.multiplicity(), qq.multi());
        EXPECT_EQ(q.is_fermion(), qq.fermion());
        // setter
        q.set_n(qq2.n);
        EXPECT_EQ(q.n(), qq2.n);
        if ((qq2.n & 1) == (qq.n & 1))
            EXPECT_EQ(q.twos(), qq.twos);
        EXPECT_EQ(q.pg(), qq.pg);
        q.set_twos(qq2.twos);
        EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq2.twos);
        EXPECT_EQ(q.pg(), qq.pg);
        EXPECT_EQ(q.multiplicity(), qq2.multi());
        EXPECT_EQ(q.is_fermion(), qq2.fermion());
        q.set_pg(qq2.pg);
        EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq2.twos);
        EXPECT_EQ(q.pg(), qq2.pg);
        EXPECT_EQ(q.multiplicity(), qq2.multi());
        EXPECT_EQ(q.is_fermion(), qq2.fermion());
        // setter different order
        q.set_twos(qq3.twos);
        if ((qq3.n & 1) == (qq2.n & 1))
            EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq3.twos);
        EXPECT_EQ(q.pg(), qq2.pg);
        q.set_pg(qq.pg);
        if ((qq3.n & 1) == (qq2.n & 1))
            EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq3.twos);
        EXPECT_EQ(q.pg(), qq.pg);
        q.set_n(qq3.n);
        EXPECT_EQ(q.n(), qq3.n);
        EXPECT_EQ(q.twos(), qq3.twos);
        EXPECT_EQ(q.pg(), qq.pg);
        EXPECT_EQ(q.multiplicity(), qq3.multi());
        EXPECT_EQ(q.is_fermion(), qq3.fermion());
        q.set_pg(qq3.pg);
        // negate
        if ((-qq3).in_range()) {
            EXPECT_EQ((-q).n(), (-qq3).n);
            EXPECT_EQ((-q).twos(), (-qq3).twos);
            EXPECT_EQ((-q).pg(), (-qq3).pg);
        }
        // addition
        S q2(qq2.n, qq2.twos, qq2.pg);
        QZLabel<S> qq4 = qq2 + qq3;
        if (qq4.in_range()) {
            EXPECT_EQ((q + q2).n(), qq4.n);
            EXPECT_EQ((q + q2).twos(), qq4.twos);
            EXPECT_EQ((q + q2).pg(), qq4.pg);
        }
        // subtraction
        QZLabel<S> qq5 = qq3 - qq2;
        if (qq5.in_range()) {
            EXPECT_EQ((q - q2).n(), qq5.n);
            EXPECT_EQ((q - q2).twos(), qq5.twos);
            EXPECT_EQ((q - q2).pg(), qq5.pg);
        }
    }
};

template <typename S> struct QULabel {
    const static int nmin, nmax, tsmin, tsmax;
    int n, twos, twosl, pg;
    QULabel() {
        n = Random::rand_int(nmin, nmax + 1);
        twos = Random::rand_int(tsmin, tsmax + 1);
        twosl = Random::rand_int(tsmin, tsmax + 1);
        pg = Random::rand_int(0, 8);
        if ((n & 1) != (twos & 1))
            twos = (twos & (~1)) | (n & 1);
        if ((n & 1) != (twosl & 1))
            twosl = (twosl & (~1)) | (n & 1);
    }
    QULabel(int n, int twosl, int twos, int pg)
        : n(n), twosl(twosl), twos(twos), pg(pg) {}
    bool in_range() const {
        return n >= nmin && n <= nmax && twos >= tsmin && twos <= tsmax &&
               twosl >= tsmin && twosl <= tsmax;
    }
    int multi() const { return twos + 1; }
    int fermion() const { return n & 1; }
    QULabel<S> operator-() const { return QULabel<S>(-n, twosl, twos, pg); }
    QULabel<S> operator+(QULabel<S> other) const {
        return QULabel<S>(n + other.n, abs(twos - other.twos),
                          twos + other.twos, pg ^ other.pg);
    }
    QULabel<S> operator-(QULabel<S> other) const { return *this + (-other); }
    QULabel<S> operator[](int i) const {
        return QULabel<S>(n, twosl + i * 2, twosl + i * 2, pg);
    }
    QULabel<S> get_ket() const { return QULabel<S>(n, twos, twos, pg); }
    QULabel<S> get_bra(QULabel<S> dq) const {
        return QULabel<S>(n + dq.n, twosl, twosl, pg ^ dq.pg);
    }
    QULabel<S> combine(QULabel<S> bra, QULabel<S> ket) const {
        return QULabel<S>(ket.n, bra.twos, ket.twos, ket.pg);
    }
    int count() const { return (twos - twosl) / 2 + 1; }
    static void check() {
        QULabel<S> qq, qq2, qq3;
        S q(qq.n, qq.twosl, qq.twos, qq.pg);
        // getter
        EXPECT_EQ(q.n(), qq.n);
        EXPECT_EQ(q.twos(), qq.twos);
        EXPECT_EQ(q.twos_low(), qq.twosl);
        EXPECT_EQ(q.pg(), qq.pg);
        EXPECT_EQ(q.multiplicity(), qq.multi());
        EXPECT_EQ(q.is_fermion(), qq.fermion());
        if (qq.twosl <= qq.twos) {
            EXPECT_EQ(q.count(), qq.count());
            int kk = Random::rand_int(0, qq.count());
            EXPECT_EQ(q[kk].n(), qq[kk].n);
            EXPECT_EQ(q[kk].twos(), qq[kk].twos);
            EXPECT_EQ(q[kk].twos_low(), qq[kk].twosl);
            EXPECT_EQ(q[kk].pg(), qq[kk].pg);
        }
        // setter
        q.set_n(qq2.n);
        EXPECT_EQ(q.n(), qq2.n);
        if ((qq2.n & 1) == (qq.n & 1)) {
            EXPECT_EQ(q.twos(), qq.twos);
            EXPECT_EQ(q.twos_low(), qq.twosl);
        }
        EXPECT_EQ(q.pg(), qq.pg);
        q.set_twos(qq2.twos);
        EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq2.twos);
        if ((qq2.n & 1) == (qq.n & 1)) {
            EXPECT_EQ(q.twos_low(), qq.twosl);
        }
        EXPECT_EQ(q.pg(), qq.pg);
        q.set_twos_low(qq2.twosl);
        EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq2.twos);
        EXPECT_EQ(q.twos_low(), qq2.twosl);
        EXPECT_EQ(q.pg(), qq.pg);
        EXPECT_EQ(q.multiplicity(), qq2.multi());
        EXPECT_EQ(q.is_fermion(), qq2.fermion());
        if (qq2.twosl <= qq2.twos) {
            EXPECT_EQ(q.count(), qq2.count());
            int kk = Random::rand_int(0, qq2.count());
            EXPECT_EQ(q[kk].n(), qq2[kk].n);
            EXPECT_EQ(q[kk].twos(), qq2[kk].twos);
            EXPECT_EQ(q[kk].twos_low(), qq2[kk].twosl);
            EXPECT_EQ(q[kk].pg(), qq.pg);
        }
        q.set_pg(qq2.pg);
        EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq2.twos);
        EXPECT_EQ(q.twos_low(), qq2.twosl);
        EXPECT_EQ(q.pg(), qq2.pg);
        EXPECT_EQ(q.multiplicity(), qq2.multi());
        EXPECT_EQ(q.is_fermion(), qq2.fermion());
        // setter different order
        q.set_twos_low(qq3.twosl);
        EXPECT_EQ(q.twos_low(), qq3.twosl);
        if ((qq3.n & 1) == (qq2.n & 1)) {
            EXPECT_EQ(q.twos(), qq2.twos);
            EXPECT_EQ(q.n(), qq2.n);
        }
        EXPECT_EQ(q.pg(), qq2.pg);
        q.set_twos(qq3.twos);
        if ((qq3.n & 1) == (qq2.n & 1)) {
            EXPECT_EQ(q.n(), qq2.n);
        }
        EXPECT_EQ(q.twos_low(), qq3.twosl);
        EXPECT_EQ(q.twos(), qq3.twos);
        EXPECT_EQ(q.pg(), qq2.pg);
        q.set_pg(qq.pg);
        if ((qq3.n & 1) == (qq2.n & 1))
            EXPECT_EQ(q.n(), qq2.n);
        EXPECT_EQ(q.twos(), qq3.twos);
        EXPECT_EQ(q.twos_low(), qq3.twosl);
        EXPECT_EQ(q.pg(), qq.pg);
        q.set_n(qq3.n);
        EXPECT_EQ(q.n(), qq3.n);
        EXPECT_EQ(q.twos_low(), qq3.twosl);
        EXPECT_EQ(q.twos(), qq3.twos);
        EXPECT_EQ(q.pg(), qq.pg);
        EXPECT_EQ(q.multiplicity(), qq3.multi());
        EXPECT_EQ(q.is_fermion(), qq3.fermion());
        q.set_pg(qq3.pg);
        // negate
        if ((-qq3).in_range()) {
            EXPECT_EQ((-q).n(), (-qq3).n);
            EXPECT_EQ((-q).twos(), (-qq3).twos);
            EXPECT_EQ((-q).twos_low(), (-qq3).twosl);
            EXPECT_EQ((-q).pg(), (-qq3).pg);
            EXPECT_EQ((-q).multiplicity(), (-qq3).multi());
            EXPECT_EQ((-q).is_fermion(), (-qq3).fermion());
            if ((-qq3).twosl <= (-qq3).twos) {
                EXPECT_EQ((-q).count(), (-qq3).count());
                int kk = Random::rand_int(0, (-qq3).count());
                EXPECT_EQ((-q)[kk].n(), (-qq3)[kk].n);
                EXPECT_EQ((-q)[kk].twos(), (-qq3)[kk].twos);
                EXPECT_EQ((-q)[kk].twos_low(), (-qq3)[kk].twosl);
                EXPECT_EQ((-q)[kk].pg(), (-qq3)[kk].pg);
            }
        }
        // addition
        q.set_twos_low(qq3.twos);
        S q2(qq2.n, qq2.twos, qq2.twos, qq2.pg);
        QULabel<S> qq4 = qq2 + qq3;
        if (qq4.in_range()) {
            EXPECT_EQ((q + q2).n(), qq4.n);
            EXPECT_EQ((q + q2).twos_low(), qq4.twosl);
            EXPECT_EQ((q + q2).twos(), qq4.twos);
            EXPECT_EQ((q + q2).pg(), qq4.pg);
            EXPECT_EQ((q + q2).multiplicity(), qq4.multi());
            EXPECT_EQ((q + q2).is_fermion(), qq4.fermion());
            if (qq4.twosl <= qq4.twos) {
                EXPECT_EQ((q + q2).count(), qq4.count());
                int kk = Random::rand_int(0, qq4.count());
                EXPECT_EQ((q + q2)[kk].n(), qq4[kk].n);
                EXPECT_EQ((q + q2)[kk].twos(), qq4[kk].twos);
                EXPECT_EQ((q + q2)[kk].twos_low(), qq4[kk].twosl);
                EXPECT_EQ((q + q2)[kk].pg(), qq4[kk].pg);
            }
        }
        // subtraction
        QULabel<S> qq5 = qq3 - qq2;
        if (qq5.in_range()) {
            EXPECT_EQ((q - q2).n(), qq5.n);
            EXPECT_EQ((q - q2).twos_low(), qq5.twosl);
            EXPECT_EQ((q - q2).twos(), qq5.twos);
            EXPECT_EQ((q - q2).pg(), qq5.pg);
            EXPECT_EQ((q - q2).multiplicity(), qq5.multi());
            EXPECT_EQ((q - q2).is_fermion(), qq5.fermion());
            if (qq5.twosl <= qq5.twos) {
                EXPECT_EQ((q - q2).count(), qq5.count());
                int kk = Random::rand_int(0, qq5.count());
                EXPECT_EQ((q - q2)[kk].n(), qq5[kk].n);
                EXPECT_EQ((q - q2)[kk].twos(), qq5[kk].twos);
                EXPECT_EQ((q - q2)[kk].twos_low(), qq5[kk].twosl);
                EXPECT_EQ((q - q2)[kk].pg(), qq5[kk].pg);
            }
        }
        // combine
        if (qq5.in_range()) {
            QULabel<S> qc = qq5.combine(qq3, qq2);
            EXPECT_EQ((q - q2).combine(q, q2).n(), qc.n);
            // EXPECT_EQ((q - q2).combine(q, q2).twos_low(), qc.twosl);
            EXPECT_EQ((q - q2).combine(q, q2).twos(), qc.twos);
            EXPECT_EQ((q - q2).combine(q, q2).pg(), qc.pg);
            EXPECT_EQ((q - q2).combine(q, q2).get_bra(q - q2).n(), qq3.n);
            EXPECT_EQ((q - q2).combine(q, q2).get_bra(q - q2).twos_low(),
                      qq3.twos);
            EXPECT_EQ((q - q2).combine(q, q2).get_bra(q - q2).twos(), qq3.twos);
            EXPECT_EQ((q - q2).combine(q, q2).get_bra(q - q2).pg(), qq3.pg);
            EXPECT_EQ((q - q2).combine(q, q2).get_ket().n(), qq2.n);
            EXPECT_EQ((q - q2).combine(q, q2).get_ket().twos_low(), qq2.twos);
            EXPECT_EQ((q - q2).combine(q, q2).get_ket().twos(), qq2.twos);
            EXPECT_EQ((q - q2).combine(q, q2).get_ket().pg(), qq2.pg);
        }
    }
};

template <> const int QZLabel<SZShort>::nmin = -128;
template <> const int QZLabel<SZShort>::nmax = 127;
template <> const int QZLabel<SZShort>::tsmin = -128;
template <> const int QZLabel<SZShort>::tsmax = 127;

template <> const int QZLabel<SZLong>::nmin = -16384;
template <> const int QZLabel<SZLong>::nmax = 16383;
template <> const int QZLabel<SZLong>::tsmin = -16384;
template <> const int QZLabel<SZLong>::tsmax = 16383;

template <> const int QULabel<SU2Short>::nmin = -128;
template <> const int QULabel<SU2Short>::nmax = 127;
template <> const int QULabel<SU2Short>::tsmin = 0;
template <> const int QULabel<SU2Short>::tsmax = 127;

template <> const int QULabel<SU2Long>::nmin = -1024;
template <> const int QULabel<SU2Long>::nmax = 1023;
template <> const int QULabel<SU2Long>::tsmin = 0;
template <> const int QULabel<SU2Long>::tsmax = 1023;

TEST_F(TestQ, TestSZShort) {
    for (int i = 0; i < n_tests; i++)
        QZLabel<SZShort>::check();
}

TEST_F(TestQ, TestSZLong) {
    for (int i = 0; i < n_tests; i++)
        QZLabel<SZLong>::check();
}

TEST_F(TestQ, TestSU2Short) {
    for (int i = 0; i < n_tests; i++)
        QULabel<SU2Short>::check();
}

TEST_F(TestQ, TestSU2Long) {
    for (int i = 0; i < n_tests; i++)
        QULabel<SU2Long>::check();
}