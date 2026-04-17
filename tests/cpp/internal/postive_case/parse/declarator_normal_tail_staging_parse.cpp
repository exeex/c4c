// Parse-only: plain declarator tails should keep qualified-name capture and
// array suffix staging stable during helper extraction.
// RUN: %c4cll --parse-only %s

struct Matrix {
    static int rows[2][3];
    int values[3];
    int operator()(int index);
};

int Matrix::rows[2][3] = {{1, 2, 3}, {4, 5, 6}};

int Matrix::operator()(int index) {
    return rows[0][index] + values[index];
}

int plain_slots[4];
int consume_named_transform(int transform(int), int value);
int consume_abstract_transform(int(int), int value);

int main() {
    Matrix matrix = {{7, 8, 9}};
    plain_slots[0] = Matrix::rows[1][2];
    return matrix(1) + plain_slots[0];
}
