#include "stdafx.h"
#include "FlannBasedTracker.h"

void FlannBasedTracker::updateWithContours(std::vector<std::vector<cv::Point> > &contours)
{
	// get internal values from slider inputs
	double minSize = (double)minSizeInput / 10.0;
	double maxSize = (double)maxSizeInput / 10.0;
	double maxJump = (double)maxJumpInput / 10.0;


	std::vector<cv::Point2f> newContourCenters;

	//extract center of Mass
	/// Get the moments of the contours
	// first kick out everything that is too small or too big, then
	// calculate their center of mass from them
	for (int i = 0; i < contours.size(); i++)
	{
		// we do this using the "moments" of the contours
		cv::Moments mu = moments(contours[i], false);
		double size = mu.m00;
		if (size >= minSize&&size <= maxSize) {
			newContourCenters.push_back(cv::Point2f(mu.m10 / mu.m00, mu.m01 / mu.m00));
		}
	}

	// prepare matrices for old+new Positions. We will use them as "features" for the point matching
	cv::Mat oldPointFeatures(trackedObjects.size(), 2, CV_32F);
	for (int i = 0; i < trackedObjects.size(); i++) {
		oldPointFeatures.at<float>(i, 0) = trackedObjects[i].lastPosition.x;
		oldPointFeatures.at<float>(i, 1) = trackedObjects[i].lastPosition.y;
	}

	cv::Mat newPointFeatures(newContourCenters.size(), 2, CV_32F);
	for (int i = 0; i < newContourCenters.size(); i++) {
		newPointFeatures.at<float>(i, 0) = newContourCenters[i].x;
		newPointFeatures.at<float>(i, 1) = newContourCenters[i].y;
	}
	// Use FLANN to match the two sets of points
	cv::FlannBasedMatcher matcher;
	std::vector< cv::DMatch > matches;
	matcher.match(newPointFeatures, oldPointFeatures, matches); //new = "query", old= "train"

	std::vector<int> contourHasMatched; // flag: did the contour match to a known object?
	contourHasMatched.resize(newContourCenters.size());
	// update all tracked objects that have been matched with a contour in the current image
	for (int matchIdx = 0; matchIdx < matches.size(); matchIdx++) {
		cv::DMatch &curMatch = matches[matchIdx];
		if (curMatch.distance < maxJump) {
			trackedObjects[curMatch.trainIdx].lastSeenInCycle = curCycle;
			contourHasMatched[curMatch.queryIdx] = 1;
			trackedObjects[curMatch.trainIdx].lastMove = (newContourCenters[curMatch.queryIdx] - trackedObjects[curMatch.trainIdx].lastPosition);
			trackedObjects[curMatch.trainIdx].lastPosition = newContourCenters[curMatch.queryIdx];
		}
	}
	// add all countours that have not matched as new objects
	for (int i = 0; i < contours.size(); i++) {
		if (contourHasMatched[i] == 0) {
			TrackedObject newObject;
			newObject.lastSeenInCycle = curCycle;
			newObject.lastPosition = newContourCenters[i];
			trackedObjects.push_back(newObject);
		}
	}

	curCycle++;
};

void FlannBasedTracker::createGui()
{
	cv::createTrackbar("minSize", "control", &minSizeInput, 50, 0);
	cv::createTrackbar("maxSize", "control", &maxSizeInput, 500, 0);
	cv::createTrackbar("maxJump", "control", &maxJumpInput, 100, 0);
}
