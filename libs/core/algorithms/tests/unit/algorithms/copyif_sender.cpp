//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, (std::numeric_limits<int>::max)());

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_copy_if_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> c(10007);
    std::vector<int> d(c.size());
    auto middle = std::begin(c) + c.size() / 2;
    std::iota(std::begin(c), middle, dis(gen));
    std::fill(middle, std::end(c), -1);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    ex::just(iterator(std::begin(c)), iterator(std::end(c)), std::begin(d),
            [](int i) { return !(i < 0); })
        | hpx::copy_if(ex_policy.on(exec))
        | tt::sync_wait();

    std::size_t count = 0;
    HPX_TEST(std::equal(
        std::begin(c), middle, std::begin(d), [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c), std::begin(d) + d.size() / 2,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template<typename IteratorTag>
void copy_if_sender_test()
{
    using namespace hpx::execution;
    test_copy_if_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_copy_if_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_copy_if_sender(hpx::launch::async, par(task), IteratorTag());
    test_copy_if_sender(hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    copy_if_sender_test<std::forward_iterator_tag>();
    copy_if_sender_test<std::random_access_iterator_tag>();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
