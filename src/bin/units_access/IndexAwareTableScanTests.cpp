// Copyright (c) 2012 Hasso-Plattner-Institut fuer Softwaresystemtechnik GmbH. All rights reserved.
#include "access/IndexAwareTableScan.h"
#include "access/CreateGroupkeyIndex.h"
#include "access/CreateDeltaIndex.h"
#include <access.h>
#include "helper/types.h"
#include "io/shortcuts.h"
#include "testing/test.h"
#include "helper/checked_cast.h"
#include "access/InsertScan.h"
#include "storage/Store.h"
#include "io/TransactionManager.h"
#include "access/expressions/pred_LessThanExpression.h"
#include "testing/TableEqualityTest.h"

namespace hyrise {
namespace access {

class IndexAwareTableScanTests : public AccessTest {
public:
  IndexAwareTableScanTests() {}

  virtual void SetUp() {
    AccessTest::SetUp();
    t = Loader::shortcuts::load("test/index_test.tbl");

    CreateGroupkeyIndex ci;
    ci.addInput(t);
    ci.addField(0);
    ci.setIndexName("idx__foo__col_0");
    ci.execute();

    CreateDeltaIndex cd;
    cd.addInput(t);
    cd.addField(0);
    cd.setIndexName("idx_delta__foo__col_0");
    cd.execute();

    CreateGroupkeyIndex ci2;
    ci2.addInput(t);
    ci2.addField(1);
    ci2.setIndexName("idx__foo__col_1");
    ci2.execute();

    CreateDeltaIndex cd2;
    cd2.addInput(t);
    cd2.addField(1);
    cd2.setIndexName("idx_delta__foo__col_1");
    cd2.execute();

    CreateGroupkeyIndex ci3;
    ci3.addInput(t);
    ci3.addField(3);
    ci3.setIndexName("idx__foo__col_3");
    ci3.execute();

    CreateDeltaIndex cd3;
    cd3.addInput(t);
    cd3.addField(3);
    cd3.setIndexName("idx_delta__foo__col_3");
    cd3.execute();

    auto row = Loader::shortcuts::load("test/index_insert_test.tbl");
    storage::atable_ptr_t table(new storage::Store(row));
    auto ctx = tx::TransactionManager::getInstance().buildContext();
    InsertScan ins;
    ins.setTXContext(ctx);
    ins.addInput(t);
    ins.setInputData(row);
    ins.execute();
  }

  storage::atable_ptr_t t;
};

TEST_F(IndexAwareTableScanTests, basic_index_aware_table_scan_test_eq) {
  auto reference = Loader::shortcuts::load("test/reference/index_aware_test_result.tbl");

  IndexAwareTableScan is;
  is.addInput(t);
  is.setTableName("foo");
  is.addField("col_0");
  is.setPredicate(new GenericExpressionValue<hyrise_int_t, std::equal_to<hyrise_int_t>>(0, "col_0", 200));
  is.execute();
  auto result = is.getResultTable();
  ASSERT_TABLE_EQUAL(result, reference);
}

TEST_F(IndexAwareTableScanTests, basic_index_aware_table_scan_test_lt) {
  auto reference = Loader::shortcuts::load("test/reference/index_aware_test_result_lt.tbl");

  IndexAwareTableScan is;
  is.addInput(t);
  is.setTableName("foo");
  is.addField("col_0");
  is.setPredicate(new GenericExpressionValue<hyrise_int_t, std::less<hyrise_int_t>>(0, "col_0", 200));
  is.execute();
  auto result = is.getResultTable();
  EXPECT_RELATION_EQ(result, reference);
}

TEST_F(IndexAwareTableScanTests, basic_index_aware_table_scan_test_intersect) {
  auto reference = Loader::shortcuts::load("test/reference/index_aware_test_result_intersect.tbl");

  IndexAwareTableScan is;
  is.addInput(t);
  is.setTableName("foo");
  is.addField("col_0");
  CompoundExpression *ce1 = new CompoundExpression(AND);
  ce1->add(new GenericExpressionValue<hyrise_int_t, std::greater<hyrise_int_t>>(0, "col_0", 110));
  ce1->add(new GenericExpressionValue<hyrise_float_t, std::less<hyrise_float_t>>(0, "col_1", 200.0));
  CompoundExpression *ce2 = new CompoundExpression(AND);
  ce2->add(ce1);
  ce2->add(new GenericExpressionValue<hyrise_int_t, std::less<hyrise_int_t>>(0, "col_3", 1000));
  is.setPredicate(ce2);
  is.execute();
  auto result = is.getResultTable();
  EXPECT_RELATION_EQ(result, reference);
}

} // namespace access
} // namespace hyrise
