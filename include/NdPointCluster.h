#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "CacheBitmap.h"


class NdPointCluster
{   
public:
    // points: a vector of points, each point is a vector of doubles
    vector<vector<int>> points;
    
    vector<vector<double>> centroids;

    // initial the points by CacheBitmap dataDims and data
    void initPoints(CacheBitmap& cacheBitmap) {
        points.clear();
        for (int i = 0; i < cacheBitmap.data.size(); i++) {
            if (cacheBitmap.data[i]) {
                vector<int> point;
                int index = i;
                for (int j = 0; j < cacheBitmap.dataDims.size(); j++) {
                    point.push_back(index % cacheBitmap.dataDims[j]);
                    index /= cacheBitmap.dataDims[j];
                }
                points.push_back(point);
            }
        }
    }
    

    double cityblock_distance(const vector<double>& x, const vector<double>& y) {
        double distance = 0.0;
        for (int i = 0; i < x.size(); i++) {
            distance += abs(x[i] - y[i]);
        }
        return distance;
    }

    vector<vector<double>> k_means_clustering(const vector<vector<double>>& points, int k, int max_iterations = 100) {
        // Initialize centroids randomly
        vector<int> centroid_indices(points.size());
        for (int i = 0; i < points.size(); i++) {
            centroid_indices[i] = i;
        }
        random_shuffle(centroid_indices.begin(), centroid_indices.end());
        // random choice of k points as centroids
        for (int i = 0; i < k; i++) {
            centroids.push_back(points[centroid_indices[i]]);
        }
        
        for (int iteration = 0; iteration < max_iterations; iteration++) {
            // Assign each point to the closest centroid
            vector<vector<vector<double>>> clusters(k);
            for (const auto& point : points) {
                double min_distance = INFINITY;
                int closest_centroid_index = -1;
                for (int i = 0; i < k; i++) {
                    double distance = cityblock_distance(point, centroids[i]);
    //                std::cout << "distance: " << distance << std::endl;
                    if (distance < min_distance) {
                        min_distance = distance;
                        closest_centroid_index = i;
                    }
                }
                clusters[closest_centroid_index].push_back(point);
            }

            
            // Update centroids to be the mean of the points in each cluster
            vector<vector<double>> new_centroids;
            for (int i = 0; i < k; i++) {
                if (clusters[i].size() > 0) {
                    vector<double> new_centroid(points[0].size(), 0.0);
                    for (const auto& point : clusters[i]) {
                        for (int j = 0; j < point.size(); j++) {
                            new_centroid[j] += point[j];
                        }
                    }
                    for (int j = 0; j < new_centroid.size(); j++) {
                        new_centroid[j] /= clusters[i].size();
                    }
                    new_centroids.push_back(new_centroid);
                }
            }
            
            // If no points were assigned to a centroid, keep the old centroid
            while (new_centroids.size() < k) {
                new_centroids.push_back(centroids[new_centroids.size()]);
            }
            
            // Check if the centroids have converged
            bool centroids_converged = true;
            for (int i = 0; i < k; i++) {
                if (cityblock_distance(centroids[i], new_centroids[i]) > 1e-6) {
                    centroids_converged = false;
                    break;
                }
            }
            if (centroids_converged) {
                break;
            }
            
            centroids = new_centroids;
        }
        
        return centroids;
    }
};