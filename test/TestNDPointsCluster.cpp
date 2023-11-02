#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace std;

double cityblock_distance(const vector<double>& x, const vector<double>& y) {
    double distance = 0.0;
    for (int i = 0; i < x.size(); i++) {
        distance += abs(x[i] - y[i]);
    }
    return distance;
}

vector<vector<double>> k_means_clustering(const vector<vector<double>>& points, int k, int max_iterations = 100) {
    // Initialize centroids randomly
    vector<vector<double>> centroids;
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

int main() {
    // Generate example points
    // vector<vector<double>> points = {
    //     {1.0, 2.0},
    //     {2.0, 1.0},
    //     {2.0, 3.0},
    //     {3.0, 2.0},
    //     {5.0, 4.0},
    //     {6.0, 5.0},
    //     {7.0, 6.0},
    //     {8.0, 5.0},
    //     {9.0, 4.0},
    //     {10.0, 5.0},
    //     {11.0, 6.0},
    //     {12.0, 5.0},
    //     {13.0, 4.0},
    //     {14.0, 5.0},
    //     {15.0, 6.0},
    //     {16.0, 5.0},
    //     {17.0, 4.0},
    //     {18.0, 5.0},
    //     {19.0, 6.0},
    //     {20.0, 5.0}
    // };

    // Generate example points (3 dimensions)
    vector<vector<double>> points = {
        {1.0, 2.0, 3.0},
        {2.0, 1.0, 3.0},
        {2.0, 3.0, 1.0},
        {3.0, 2.0, 1.0},
        {5.0, 4.0, 3.0},
        {6.0, 5.0, 3.0},
        {7.0, 6.0, 3.0},
        {8.0, 5.0, 3.0},
        {9.0, 4.0, 3.0},
        {10.0, 5.0, 3.0},
        {11.0, 6.0, 3.0},
        {12.0, 5.0, 3.0},
        {13.0, 4.0, 3.0},
        {14.0, 5.0, 3.0},
        {15.0, 6.0, 3.0},
        {16.0, 5.0, 3.0},
        {17.0, 4.0, 3.0},
        {18.0, 5.0, 3.0},
        {19.0, 6.0, 3.0},
        {20.0, 5.0, 3.0}
    };
    
    // Cluster the points into two groups
    int k = 2;
    vector<vector<double>> centroids = k_means_clustering(points, k);
    
    // Print the centroids
    cout << "Centroids:" << endl;
    for (const auto& centroid : centroids) {
        for (const auto& coordinate : centroid) {
            cout << coordinate << " ";
        }
        cout << endl;
    }
    // Print the points in each cluster
    cout << endl << "Clusters:" << endl;
    for (int i = 0; i < k; i++) {
        cout << "Cluster " << i << ": ";
        for (const auto& point : points) {
            double min_distance = INFINITY;
            int closest_centroid_index = -1;
            for (int i = 0; i < k; i++) {
                double distance = cityblock_distance(point, centroids[i]);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_centroid_index = i;
                }
            }
            if (closest_centroid_index == i) {
                cout << "(";
                for (const auto& coordinate : point) {
                    cout << coordinate << " ";
                }
                cout << ") ";
            }
        }
        cout << endl;
    }

    // Print the bounding boxes of the clusters
    cout << endl << "Bounding boxes:" << endl;
    for (int i = 0; i < k; i++) {
        double min_x = INFINITY;
        double max_x = -INFINITY;
        double min_y = INFINITY;
        double max_y = -INFINITY;
        for (const auto& point : points) {
            double min_distance = INFINITY;
            int closest_centroid_index = -1;
            for (int i = 0; i < k; i++) {
                double distance = cityblock_distance(point, centroids[i]);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_centroid_index = i;
                }
            }
            if (closest_centroid_index == i) {
                min_x = min(min_x, point[0]);
                max_x = max(max_x, point[0]);
                min_y = min(min_y, point[1]);
                max_y = max(max_y, point[1]);
            }
        }
        cout << "Cluster " << i << ": (" << min_x << ", " << min_y << ") to (" << max_x << ", " << max_y << ")" << endl;
    }

    return 0;
}