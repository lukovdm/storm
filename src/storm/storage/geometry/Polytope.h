#ifndef STORM_STORAGE_GEOMETRY_POLYTOPE_H_
#define STORM_STORAGE_GEOMETRY_POLYTOPE_H_

#include <vector>
#include <memory>
#include <boost/optional.hpp>

#include "storm/storage/geometry/Halfspace.h"

namespace storm {
    namespace storage {
        namespace geometry {
            
            template <typename ValueType>
            class Polytope {
            public:
                
                typedef std::vector<ValueType> Point;
            
                virtual ~Polytope();
                
                /*!
                 * Creates a polytope from the given halfspaces.
                 * If the given vector of halfspaces is empty, the resulting polytope is universal (i.e., equals R^n).
                 */
                static std::shared_ptr<Polytope<ValueType>> create(std::vector<Halfspace<ValueType>> const& halfspaces);
                
                /*!
                 * Creates a polytope from the given points (i.e., the convex hull of the points).
                 * If the vector of points is empty, the resulting polytope be empty.
                 */
                static std::shared_ptr<Polytope<ValueType>> create(std::vector<Point> const& points);
                
                /*!
                 * Creates the universal polytope (i.e., the set R^n)
                 */
                static std::shared_ptr<Polytope<ValueType>> createUniversalPolytope();
                
                /*!
                 * Creates the empty polytope (i.e., emptyset)
                 */
                static std::shared_ptr<Polytope<ValueType>> createEmptyPolytope();
                
                /*!
                 * Creates the downward closure of the given points (i.e., the set { x | ex. y \in conv(points) : x<=y }
                 * If the vector of points is empty, the resulting polytope be empty.
                 */
                static std::shared_ptr<Polytope<ValueType>> createDownwardClosure(std::vector<Point> const& points);
                
                /*!
                 * Returns the vertices of this polytope.
                 */
                virtual std::vector<Point> getVertices() const = 0;
                
                /*!
                 * Returns the vertices of this 2D-polytope in clockwise order.
                 * An Exception is thrown if the dimension of this polytope is not two.
                 */
                virtual std::vector<Point> getVerticesInClockwiseOrder() const;
                
                /*!
                 * Returns the halfspaces of this polytope.
                 */
                virtual std::vector<Halfspace<ValueType>> getHalfspaces() const = 0;
                
                /*!
                 * Returns whether this polytope is the empty set.
                 */
                virtual bool isEmpty() const = 0;
                
                /*!
                 * Returns whether this polytope is universal (i.e., equals R^n).
                 */
                virtual bool isUniversal() const = 0;
                
                /*!
                 * Returns true iff the given point is inside of the polytope.
                 */
                virtual bool contains(Point const& point) const = 0;
                
                /*!
                 * Returns true iff the given polytope is a subset of this polytope.
                 */
                virtual bool contains(std::shared_ptr<Polytope<ValueType>> const& other) const = 0;
                
                /*!
                 * Intersects this polytope with rhs and returns the result.
                 */
                virtual std::shared_ptr<Polytope<ValueType>> intersection(std::shared_ptr<Polytope<ValueType>> const& rhs) const = 0;
                virtual std::shared_ptr<Polytope<ValueType>> intersection(Halfspace<ValueType> const& halfspace) const = 0;
                
                /*!
                 * Returns the convex union of this polytope and rhs.
                 */
                virtual std::shared_ptr<Polytope<ValueType>> convexUnion(std::shared_ptr<Polytope<ValueType>> const& rhs) const = 0;
        
                /*!
                 * Returns the minkowskiSum of this polytope and rhs.
                 */
                virtual std::shared_ptr<Polytope<ValueType>> minkowskiSum(std::shared_ptr<Polytope<ValueType>> const& rhs) const = 0;
        
                /*!
                 * Returns the affine transformation of this polytope P w.r.t. the given matrix A and vector b.
                 * The result is the set {A*x+b | x \in P}
                 *
                 * @param matrix the transformation matrix, given as vector of rows
                 * @param vector the transformation offset
                 */
                virtual std::shared_ptr<Polytope<ValueType>> affineTransformation(std::vector<Point> const& matrix, Point const& vector) const = 0;
                
                /*!
                 * Returns the downward closure of this, i.e., the set { x | ex. y \in P : x<=y} where P is this Polytope.
                 */
                virtual std::shared_ptr<Polytope<ValueType>> downwardClosure() const;
                
                /*!
                 * Finds an optimal point inside this polytope w.r.t. the given direction, i.e.,
                 * a point that maximizes dotPorduct(point, direction).
                 * If such a point does not exist, the returned bool is false. There are two reasons for this:
                 * - The polytope is empty
                 * - The polytope is not bounded in the given direction
                 */
                virtual std::pair<Point, bool> optimize(Point const& direction) const = 0;
                
                /*!
                 * converts the intern number representation of the polytope to the given target type
                 */
                template <typename TargetType>
                std::shared_ptr<Polytope<TargetType>> convertNumberRepresentation() const;
                
                /*
                 * Returns a string representation of this polytope.
                 * @param numbersAsDouble If true, the occurring numbers are converted to double before printing to increase readability.
                 */
                virtual std::string toString(bool numbersAsDouble = false) const;
               
                virtual bool isHyproPolytope() const;

                virtual bool isNativePolytope() const;

            protected:
                
                Polytope();
                
            private:
                /*!
                 * Creates a polytope from the given halfspaces or vertices.
                 */
                static std::shared_ptr<Polytope<ValueType>> create(boost::optional<std::vector<Halfspace<ValueType>>> const& halfspaces,
                                                                   boost::optional<std::vector<Point>> const& points);
                
                
            };
            
        }
    }
}

#endif /* STORM_STORAGE_GEOMETRY_POLYTOPE_H_ */
