
/*
 * block2: Efficient MPO implementation of quantum chemistry DMRG
 * Copyright (C) 2020 Huanchen Zhai <hczhai@caltech.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "block2.hpp"

using namespace std;
using namespace block2;

map<string, string> read_input(const string &filename) {
    if (!Parsing::file_exists(filename)) {
        cerr << "cannot find input file : " << filename << endl;
        abort();
    }
    ifstream ifs(filename.c_str());
    vector<string> lines = Parsing::readlines(&ifs);
    ifs.close();
    map<string, string> params;
    for (auto x : lines) {
        vector<string> line = Parsing::split(x, "=", true);
        if (line.size() == 1)
            params[Parsing::trim(line[0])] = "";
        else if (line.size() == 2)
            params[Parsing::trim(line[0])] = Parsing::trim(line[1]);
        else if (line.size() == 0)
            continue;
        else {
            cerr << "cannot parse input : " << x << endl;
            abort();
        }
    }
    return params;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        cout << "usage : block2 <input filename>" << endl;
        abort();
    }

    string input = argv[1];

    auto params = read_input(input);

    if (params.count("rand_seed") != 0)
        Random::rand_seed(Parsing::to_int(params.at("rand_seed")));
    else
        Random::rand_seed(0);

    size_t memory = 4ULL << 30;
    if (params.count("memory") != 0)
        memory = (size_t)Parsing::to_double(params.at("memory"));

    string scratch = "./node0";
    if (params.count("scratch") != 0)
        scratch = params.at("scratch");

    frame_() =
        new DataFrame((size_t)(0.1 * memory), (size_t)(0.9 * memory), scratch);

    cout << "integer stack memory = " << fixed << setprecision(4)
         << ((frame_()->isize << 2) / 1E9) << " GB" << endl;
    cout << "double  stack memory = " << fixed << setprecision(4)
         << ((frame_()->dsize << 3) / 1E9) << " GB" << endl;

    shared_ptr<FCIDUMP> fcidump = make_shared<FCIDUMP>();
    vector<double> occs;
    PGTypes pg = PGTypes::C1;

    if (params.count("occ_file") != 0)
        occs = read_occ(params.at("occ_file"));

    if (params.count("pg") != 0) {
        string xpg = params.at("pg");
        if (xpg == "c1")
            pg = PGTypes::C1;
        else if (xpg == "c2")
            pg = PGTypes::C2;
        else if (xpg == "ci")
            pg = PGTypes::CI;
        else if (xpg == "cs")
            pg = PGTypes::CS;
        else if (xpg == "c2h")
            pg = PGTypes::C2H;
        else if (xpg == "c2v")
            pg = PGTypes::C2V;
        else if (xpg == "d2")
            pg = PGTypes::D2;
        else if (xpg == "d2h")
            pg = PGTypes::D2H;
        else {
            cerr << "unknown point group : " << xpg << endl;
            abort();
        }
    }

    if (params.count("fcidump") != 0) {
        fcidump->read(params.at("fcidump"));
    } else {
        cerr << "'ficudmp' parameter not found!" << endl;
        abort();
    }

    if (params.count("n_elec") != 0)
        fcidump->params["nelec"] = params.at("n_elec");

    if (params.count("twos") != 0)
        fcidump->params["ms2"] = params.at("twos");

    if (params.count("ipg") != 0)
        fcidump->params["isym"] = params.at("ipg");

    if (params.count("mkl_threads") != 0) {
        mkl_set_num_threads(Parsing::to_int(params.at("mkl_threads")));
        mkl_set_dynamic(1);
        cout << "using " << Parsing::to_int(params.at("mkl_threads"))
             << " mkl threads" << endl;
    }

    vector<uint8_t> orbsym = fcidump->orb_sym();
    transform(orbsym.begin(), orbsym.end(), orbsym.begin(),
              PointGroup::swap_pg(pg));
    SU2 vacuum(0);
    SU2 target(fcidump->n_elec(), fcidump->twos(),
               PointGroup::swap_pg(pg)(fcidump->isym()));
    int norb = fcidump->n_sites();
    bool su2 = !fcidump->uhf;
    HamiltonianQC<SU2> hamil(vacuum, norb, orbsym, fcidump);

    QCTypes qc_type = QCTypes::Conventional;

    if (params.count("qc_type") != 0) {
        if (params.at("qc_type") == "conventional")
            qc_type = QCTypes::Conventional;
        else if (params.at("qc_type") == "nc")
            qc_type = QCTypes::NC;
        else if (params.at("qc_type") == "cn")
            qc_type = QCTypes::CN;
        else {
            cerr << "unknown qc type : " << params.at("qc_type") << endl;
            abort();
        }
    }

    Timer t;
    t.get_time();
    // MPO construction
    cout << "MPO start" << endl;
    shared_ptr<MPO<SU2>> mpo = make_shared<MPOQC<SU2>>(hamil, qc_type);
    cout << "MPO end .. T = " << t.get_time() << endl;

    // MPO simplification
    cout << "MPO simplification start" << endl;
    mpo =
        make_shared<SimplifiedMPO<SU2>>(mpo, make_shared<RuleQC<SU2>>(), true);
    cout << "MPO simplification end .. T = " << t.get_time() << endl;

    if (params.count("print_mpo") != 0)
        cout << mpo->get_blocking_formulas() << endl;

    if (params.count("print_mpo_dims") != 0) {
        cout << "left mpo dims = ";
        for (int i = 0; i < norb; i++)
            cout << mpo->left_operator_names[i]->data.size() << " ";
        cout << endl;
        cout << "right mpo dims = ";
        for (int i = 0; i < norb; i++)
            cout << mpo->right_operator_names[i]->data.size() << " ";
        cout << endl;
    }

    vector<uint16_t> bdims = {250, 250, 250, 250, 250, 500};
    vector<double> noises = {1E-6, 1E-6, 1E-6, 1E-6, 1E-6, 0.0};
    vector<double> davidson_conv_thrds = {5E-6};

    if (params.count("bond_dims") != 0) {
        vector<string> xbdims =
            Parsing::split(params.at("bond_dims"), " ", true);
        bdims.clear();
        for (auto x : xbdims)
            bdims.push_back((uint16_t)Parsing::to_int(x));
    }

    if (params.count("noises") != 0) {
        vector<string> xnoises = Parsing::split(params.at("noises"), " ", true);
        noises.clear();
        for (auto x : xnoises)
            noises.push_back(Parsing::to_double(x));
    }

    if (params.count("davidson_conv_thrds") != 0) {
        davidson_conv_thrds.clear();
        if (params.at("davidson_conv_thrds") != "auto") {
            vector<string> xdavidson_conv_thrds =
                Parsing::split(params.at("davidson_conv_thrds"), " ", true);
            for (auto x : xdavidson_conv_thrds)
                davidson_conv_thrds.push_back(Parsing::to_double(x));
        }
    }

    hamil.opf->seq->mode = SeqTypes::Simple;

    if (params.count("seq_type") != 0) {
        if (params.at("seq_type") == "none")
            hamil.opf->seq->mode = SeqTypes::None;
        else if (params.at("seq_type") == "simple")
            hamil.opf->seq->mode = SeqTypes::Simple;
        else if (params.at("seq_type") == "auto")
            hamil.opf->seq->mode = SeqTypes::Auto;
        else {
            cerr << "unknown seq type : " << params.at("seq_type") << endl;
            abort();
        }
    }

    shared_ptr<MPSInfo<SU2>> mps_info = nullptr;

    if (params.count("casci") != 0) {
        // active sites, active electrons
        vector<string> xcasci = Parsing::split(params.at("casci"), " ", true);
        mps_info = make_shared<CASCIMPSInfo<SU2>>(
            norb, vacuum, target, hamil.basis, hamil.orb_sym,
            Parsing::to_int(xcasci[0]), Parsing::to_int(xcasci[1]));
    } else
        mps_info = make_shared<MPSInfo<SU2>>(norb, vacuum, target, hamil.basis,
                                             hamil.orb_sym);
    double bias = 1.0;

    if (params.count("occ_bias") != 0)
        bias = Parsing::to_double(params.at("occ_bias"));

    if (params.count("mps") != 0) {
        mps_info->tag = params.at("mps");
        mps_info->load_mutable();
    } else if (occs.size() == 0)
        mps_info->set_bond_dimension(bdims[0]);
    else {
        assert(occs.size() == norb);
        mps_info->set_bond_dimension_using_occ(bdims[0], occs, bias);
    }

    if (params.count("print_fci_dims") != 0) {
        cout << "left fci dims = ";
        for (int i = 0; i <= norb; i++)
            cout << mps_info->left_dims_fci[i].n_states_total << " ";
        cout << endl;
        cout << "right fci dims = ";
        for (int i = 0; i <= norb; i++)
            cout << mps_info->right_dims_fci[i].n_states_total << " ";
        cout << endl;
    }

    if (params.count("print_mps_dims") != 0) {
        cout << "left mps dims = ";
        for (int i = 0; i <= norb; i++)
            cout << mps_info->left_dims[i].n_states_total << " ";
        cout << endl;
        cout << "right mps dims = ";
        for (int i = 0; i <= norb; i++)
            cout << mps_info->right_dims[i].n_states_total << " ";
        cout << endl;
    }

    int center = 0, dot = 2;

    if (params.count("center") != 0)
        center = Parsing::to_int(params.at("center"));

    if (params.count("dot") != 0)
        dot = Parsing::to_int(params.at("dot"));
    shared_ptr<MPS<SU2>> mps = nullptr;

    if (params.count("mps") != 0) {
        mps_info->tag = params.at("mps");
        mps = make_shared<MPS<SU2>>(mps_info);
        mps->load_data();
        mps->load_mutable();
        mps_info->tag = "KET";
    } else {
        mps = make_shared<MPS<SU2>>(norb, center, dot);
        mps->initialize(mps_info);
        mps->random_canonicalize();
    }

    mps->save_mutable();
    mps->deallocate();
    mps_info->save_mutable();
    mps_info->deallocate_mutable();

    int iprint = 2;
    if (params.count("iprint") != 0)
        iprint = Parsing::to_int(params.at("iprint"));

    shared_ptr<MovingEnvironment<SU2>> me =
        make_shared<MovingEnvironment<SU2>>(mpo, mps, mps, "DMRG");
    t.get_time();
    cout << "INIT start" << endl;
    me->init_environments(iprint >= 2);
    cout << "INIT end .. T = " << t.get_time() << endl;

    int n_sweeps = 30;
    bool forward = true;
    double tol = 1E-6;

    if (params.count("n_sweeps") != 0)
        n_sweeps = Parsing::to_int(params.at("n_sweeps"));

    if (params.count("forward") != 0)
        forward = !!Parsing::to_int(params.at("forward"));

    if (params.count("tol") != 0)
        tol = Parsing::to_double(params.at("tol"));

    shared_ptr<DMRG<SU2>> dmrg = make_shared<DMRG<SU2>>(me, bdims, noises);
    dmrg->davidson_conv_thrds = davidson_conv_thrds;
    dmrg->iprint = iprint;

    if (params.count("noise_type") != 0) {
        if (params.at("noise_type") == "density_matrix")
            dmrg->noise_type = NoiseTypes::DensityMatrix;
        else if (params.at("noise_type") == "wavefunction")
            dmrg->noise_type = NoiseTypes::Wavefunction;
        else if (params.at("noise_type") == "perturbative")
            dmrg->noise_type = NoiseTypes::Perturbative;
        else {
            cerr << "unknown noise type : " << params.at("noise_type") << endl;
            abort();
        }
    }

    if (params.count("trunc_type") != 0) {
        if (params.at("trunc_type") == "physical")
            dmrg->trunc_type = TruncationTypes::Physical;
        else if (params.at("trunc_type") == "reduced")
            dmrg->trunc_type = TruncationTypes::Reduced;
        else {
            cerr << "unknown trunc type : " << params.at("trunc_type") << endl;
            abort();
        }
    }

    if (params.count("cutoff") != 0)
        dmrg->cutoff = Parsing::to_double(params.at("cutoff"));

    dmrg->solve(n_sweeps, forward, tol);

    mps->save_data();

    mps_info->deallocate();
    mpo->deallocate();
    hamil.deallocate();
    fcidump->deallocate();

    frame_()->activate(0);
    assert(ialloc_()->used == 0 && dalloc_()->used == 0);
    delete frame_();

    return 0;
}