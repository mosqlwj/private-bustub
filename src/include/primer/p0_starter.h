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
  // TODO(P0): Add implementation
  Matrix(int r, int c) : rows(r), cols(c), linear(new T[r * c]) {
    for (int i = 0; i < r * c; i++) {
      linear[i] = 0;
    }
  }

  // # of rows in the matrix
  int rows;
  // # of Columns in the matrix
  int cols;
  // Flattened array containing the elements of the matrix
  // TODO(P0) : Allocate the array in the constructor. Don't forget to free up
  // the array in the destructor.
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

  // TODO(P0): Add implementation
  virtual ~Matrix() { delete[](linear); }
};

template <typename T>
class RowMatrix : public Matrix<T> {
 public:
  // TODO(P0): Add implementation
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

  // TODO(P0): Add implementation
  int GetRows() override { return this->rows; }

  // TODO(P0): Add implementation
  int GetColumns() override { return this->cols; }

  // TODO(P0): Add implementation
  T GetElem(int i, int j) override {
    if (i < 0 || i >= GetRows() || j < 0 || j >= GetColumns()) {
      return -1;
    }
    return data_[i][j];
  }

  // TODO(P0): Add implementation
  void SetElem(int i, int j, T val) override {
    if (i < 0 || i >= GetRows() || j < 0 || j >= GetColumns()) {
      return;
    }
    data_[i][j] = val;
    this->linear[i * GetColumns() + j] = val;
  }

  // TODO(P0): Add implementation
  void MatImport(T *arr) override {
    // 是用arr指向的内容填写本矩阵的内容？
    int index = 0;
    for (int i = 0; i < GetRows(); i++) {
      for (int j = 0; j < GetColumns(); j++) {
        this->linear[index] = arr[index];
        data_[i][j] = arr[index];
        index++;
      }
    }
  }

  // TODO(P0): Add implementation
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
    // TODO(P0): Add code
    if (mat1->GetRows() != mat2->GetRows() || mat1->GetColumns() != mat2->GetColumns()) {
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
  // 注意矩阵乘法是MxN * N*M 得到M*M，所以要看匹不匹配是看前一个的列和后一个的行是否匹配
  static std::unique_ptr<RowMatrix<T>> MultiplyMatrices(std::unique_ptr<RowMatrix<T>> mat1,
                                                        std::unique_ptr<RowMatrix<T>> mat2) {
    // TODO(P0): Add code
    if (mat1->GetRows() != mat2->GetColumns()) {
      return std::unique_ptr<RowMatrix<T>>(nullptr);
    }
    std::unique_ptr<RowMatrix<T>> mat(new RowMatrix<T>(mat1->GetRows(), mat1->GetColumns()));
    for (int i = 0; i < mat1->GetRows(); i++) {
      for (int j = 0; j < mat2->GetColumns(); j++) {
        for (int k = 0; k < mat1->GetColumns(); k++) {
          // fprintf(stderr,"mat(%d,%d) %d +mat1(%d,%d) %d * mat2(%d,%d) %d
          // ",i,j,mat->GetElem(i,j),i,k,mat1->GetElem(i,k),k,j,mat2->GetElem(k,j));
          mat->SetElem(i, j, mat->GetElem(i, j) + mat1->GetElem(i, k) * mat2->GetElem(k, j));
          // fprintf(stderr,"%d\n",mat->GetElem(i,j));
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
    // TODO(P0): Add code
    std::unique_ptr<RowMatrix<T>> rs1(std::move(MultiplyMatrices(matA, matB)));
    if (rs1 == nullptr) {
      return rs1;
    }
    std::unique_ptr<RowMatrix<T>> rs2(std::move(AddMatrices(rs1, matC)));
    return rs2;
  }
};

}  // namespace bustub
