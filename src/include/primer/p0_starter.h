//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// p0_starter.h
//
// Identification: src/include/primer/p0_starter.h
//
// Copyright (c) 2015-2020, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
namespace bustub {

/*
 * The base class defining a Matrix
 */
template <typename T>
class Matrix {
 protected:
  // NOTE: 初始化列表中初始化的顺序应该和变量在类中声明的顺序相同
  Matrix(int r, int c) : rows(r), cols(c), linear(new T[r * c]) {
    for (int i = 0; i < r * c; i++) {
      linear[i] = 0;
    }
  }

  // # of rows in the matrix
  int rows;
  // # of Columns in the matrix
  int cols;
  // NOTE:在构造函数中申请，在析构函数中释放
  T *linear;

 public:
  // Return the # of rows in the matrix
  virtual int GetRows() = 0;

  // Return the # of columns in the matrix
  virtual int GetColumns() = 0;

  // Return the (i,j)th  matrix element
  virtual T GetElem(int i, int j) = 0;

  // Sets the (i,j)th  matrix element to val
  virtual void SetElem(int i, int j, T val) = 0;

  // Sets the matrix elements based on the array arr
  virtual void MatImport(T *arr) = 0;

  // NOTE: delete释放单个对象和释放数组的区别，释放数组是delete[]
  virtual ~Matrix() { delete[](linear); }
};

template <typename T>
class RowMatrix : public Matrix<T> {
 public:
  // NOTE: 动态申请二维数组，是通过一个一维的指针数组，然后为每个指针申请对应的行数组
  RowMatrix(int r, int c) : Matrix<T>(r, c) {
    data_ = new int *[r];
    for (int i = 0; i < r; i++) {
      data_[i] = new int[c];
    }
    for (int i = 0; i < r; i++) {
      for (int j = 0; j < c; j++) {
        data_[i][j] = 0;
      }
    }
  }

  // TODO: 是否是因为访问基类的成员变量，所以要用this?
  int GetRows() override { return this->rows; }

  // TODO:同上
  int GetColumns() override { return this->cols; }

  T GetElem(int i, int j) override {
    if (i < 0 || i >= GetRows() || j < 0 || j >= GetColumns()) {
      return -1;
    }
    return data_[i][j];
  }

  void SetElem(int i, int j, T val) override {
    if (i < 0 || i >= GetRows() || j < 0 || j >= GetColumns()) {
      return;
    }
    data_[i][j] = val;
    this->linear[i * GetColumns() + j] = val;
  }

  void MatImport(T *arr) override {
    int index = 0;
    for (int i = 0; i < GetRows(); i++) {
      for (int j = 0; j < GetColumns(); j++) {
        this->linear[index] = arr[index];
        data_[i][j] = arr[index];
        index++;
      }
    }
  }

  // TODO: 不用显式调用基类的析构函数？
  ~RowMatrix() override {
    // ~Matrix<T>();
    for (int i = 0; i < GetRows(); i++) {
      delete[](data_[i]);
    }
    delete[](data_);
  }

 private:
  // 2D array containing the elements of the matrix in row-major format
  // TODO(P0): Allocate the array of row pointers in the constructor. Use these pointers
  // to point to corresponding elements of the 'linear' array.
  // Don't forget to free up the array in the destructor.
  T **data_;
};

template <typename T>
class RowMatrixOperations {
 public:
  // Compute (mat1 + mat2) and return the result.
  // Return nullptr if dimensions mismatch for input matrices.
  static std::unique_ptr<RowMatrix<T>> AddMatrices(std::unique_ptr<RowMatrix<T>> mat1,
                                                   std::unique_ptr<RowMatrix<T>> mat2) {
    if (mat1->GetRows() != mat2->GetRows() || mat1->GetColumns() != mat2->GetColumns()) {
      fprintf(stderr,"can not add %d %d\n",mat1->GetRows(),mat1->GetColumns());
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }
    std::unique_ptr<RowMatrix<T>> mat(new RowMatrix<T>(mat1->GetRows(), mat1->GetColumns()));
    for (int i = 0; i < mat->GetRows(); i++) {
      for (int j = 0; j < mat->GetColumns(); j++) {
        mat->SetElem(i, j, mat1->GetElem(i, j) + mat2->GetElem(i, j));
      }
    }
    return mat;
  }

  // Compute matrix multiplication (mat1 * mat2) and return the result.
  // Return nullptr if dimensions mismatch for input matrices.
  // 矩阵乘法是MxN * N*M 得到M*M，所以要看匹不匹配是看前一个的列和后一个的行是否匹配
  static std::unique_ptr<RowMatrix<T>> MultiplyMatrices(std::unique_ptr<RowMatrix<T>> mat1,
                                                        std::unique_ptr<RowMatrix<T>> mat2) {
    if (mat1->GetColumns() != mat2->GetRows()) {
      fprintf(stderr,"can not multiply\n");
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }
    std::unique_ptr<RowMatrix<T>> mat(new RowMatrix<T>(mat1->GetRows(), mat2->GetColumns()));
    for (int i = 0; i < mat1->GetRows(); i++) {
      for (int j = 0; j < mat2->GetColumns(); j++) {
        for (int k = 0; k < mat1->GetColumns(); k++) {
          // fprintf(stderr,"mat(%d,%d) %d +mat1(%d,%d) %d * mat2(%d,%d) %d
          // ",i,j,mat->GetElem(i,j),i,k,mat1->GetElem(i,k),k,j,mat2->GetElem(k,j));
          mat->SetElem(i, j, mat->GetElem(i, j) + mat1->GetElem(i, k) * mat2->GetElem(k, j));
        }
      }
    }
    return mat;
  }

  // Simplified GEMM (general matrix multiply) operation
  // Compute (matA * matB + matC). Return nullptr if dimensions mismatch for input matrices
  static std::unique_ptr<RowMatrix<T>> GemmMatrices(std::unique_ptr<RowMatrix<T>> matA,
                                                    std::unique_ptr<RowMatrix<T>> matB,
                                                    std::unique_ptr<RowMatrix<T>> matC) {
    // NOTE: unique_ptr不能被复制，只能被move
    std::unique_ptr<RowMatrix<T>> rs1(std::move(MultiplyMatrices(std::move(matA), std::move(matB))));
    if (rs1 == nullptr) {
      fprintf(stderr,"multiply rs is null\n");
      return rs1;
    }
    std::unique_ptr<RowMatrix<T>> rs2(std::move(AddMatrices(std::move(rs1), std::move(matC))));
    if (rs2 == nullptr) {
      fprintf(stderr,"add rs is null\n");
      return rs2;
    }
    return rs2;
  }
};

}  // namespace bustub
