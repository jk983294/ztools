#include <memory.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <zerg/io/archiver.h>

using namespace std;
using namespace zerg;

/**
 * save MyObj1 with member function, save member by member
 */
struct MyObj1 {
    int a;
    CHECKPOINT_SUPPORT(a)
};

/**
 * save whole MyObj2, define overload function
 */
struct MyObj2 {
    int a;
};

CHECKPOINT_SUPPORT_STRUCT_DECLARE(MyObj2)
CHECKPOINT_SUPPORT_STRUCT(MyObj2)

struct MyClass1 {
    int var1, var2;
    MyObj1 obj1;
    MyObj2 obj2;
    void checkpoint_save(ArchiverWriter &writer) const { writer *var1 *var2 *obj1 *obj2; }
    void checkpoint_load(ArchiverReader &reader) { reader *var1 *var2 *obj1 *obj2; }
    void checkpoint_bytes(ArchiverSizer &sizer) const { sizer *var1 *var2 *obj1 *obj2; }
    //    CHECKPOINT_SUPPORT(var1 * var2 * obj1 * obj2)
};

struct MyBase {
    MyBase() = default;
    virtual ~MyBase() = default;
    int var1{0};
    int var3{0};

    virtual void sayType() {
        printf("MyBase::sayType\n");
    }

    inline virtual void checkpoint_save(ArchiverWriter &writer) const { writer *var1; }
    inline virtual void checkpoint_load(ArchiverReader &reader) { reader *var1; }
    inline virtual void checkpoint_bytes(ArchiverSizer &sizer) const { sizer *var1; }
    // CHECKPOINT_SUPPORT_VIRTUAL(var1)
};

struct MyDerive : public MyBase {
    int var2{0};
    int var4{0};

    virtual void sayType() {
        printf("MyDerive::sayType\n");
    }

    inline virtual void checkpoint_save(ArchiverWriter &writer) const {
        MyBase::checkpoint_save(writer);
        writer *var2;
    }
    inline virtual void checkpoint_load(ArchiverReader &reader) {
        MyBase::checkpoint_load(reader);
        reader *var2;
    }
    inline virtual void checkpoint_bytes(ArchiverSizer &sizer) const {
        MyBase::checkpoint_bytes(sizer);
        sizer *var2;
    }
    // CHECKPOINT_SUPPORT_DERIVED(MyBase, var2)
};

void basic_demo() {
    ArchiverSizer sizer;
    MyClass1 x;
    x.var1 = 1;
    x.var2 = 2;
    x.obj1.a = 3;
    x.obj2.a = 4;

    ArchiverWriter writer;
    sizer *x;  // read size of x
    writer.alloc_mem(sizer.getTotalSize());
    writer *x;  // save x into writer

    MyClass1 y;
    ArchiverReader reader;
    reader.read_from_memory(writer.getDataPointer(), writer.getTotalSize());
    reader *y;  // write data into y
    cout << y.var1 << " " << y.var2 << " " << y.obj1.a << " " << y.obj2.a << endl;
}

void derive_demo() {
    ArchiverSizer sizer;
    MyDerive x;
    x.var1 = 1;
    x.var2 = 2;

    ArchiverWriter writer;
    sizer *x;  // read size of x
    writer.alloc_mem(sizer.getTotalSize());
    writer *x;  // save x into writer

    MyDerive y;
    ArchiverReader reader;
    reader.read_from_memory(writer.getDataPointer(), writer.getTotalSize());
    reader *y;  // write data into y
    cout << y.var1 << " " << y.var2 << endl;
}

void polymorphism_demo() {
    ArchiverSizer sizer;
    MyBase* a = new MyDerive;
    a->var1 = 1;
    a->var3 = 3;
    ((MyDerive*)a)->var2 = 2;
    ((MyDerive*)a)->var4 = 4;

    ArchiverWriter writer;
    sizer *(*a);  // read size of x
    writer.alloc_mem(sizer.getTotalSize());
    writer *(*a);  // save x into writer

    MyBase* b = new MyDerive;
    MyDerive* b_cast = (MyDerive*)(b);
    b->var3 = 42;
    ArchiverReader reader;
    reader.read_from_memory(writer.getDataPointer(), writer.getTotalSize());
    reader *(*b);
    printf("from %d,%d,%d,%d\n", b->var1, b->var3, b_cast->var2, b_cast->var4);
    b->sayType();
}

struct TestData {
    int data{-1};
};

CHECKPOINT_SUPPORT_STRUCT_DECLARE(TestData)
CHECKPOINT_SUPPORT_STRUCT(TestData)

struct MyStdObj {
    map<string, string> a;
    vector<int> b;
    deque<double> c;
    list<float> d;
    vector<vector<TestData>> e;
    CHECKPOINT_SUPPORT(a *b *c *d *e)
};

void std_demo() {
    ArchiverSizer sizer;
    MyStdObj x;
    x.a["1"] = "1";
    x.b.push_back(2);
    x.c.push_back(3.0);
    x.d.push_back(4.0f);
    x.e.resize(1);
    x.e.back().push_back({42});

    ArchiverWriter writer;
    sizer *x;  // read size of x
    writer.alloc_mem(sizer.getTotalSize());
    writer *x;  // save x into writer

    MyStdObj y;
    ArchiverReader reader;
    reader.read_from_memory(writer.getDataPointer(), writer.getTotalSize());
    reader *y;  // write data into y
    cout << y.a["1"] << " " << y.b.front() << " " << y.c.front() << " " << y.d.front() << " " << y.e.front().front().data << endl;
}

int main() {
    basic_demo();
    derive_demo();
    polymorphism_demo();
    std_demo();
    return 0;
}
