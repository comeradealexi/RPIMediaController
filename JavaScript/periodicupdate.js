function myPeriodicMethod() {
  $.ajax({
    url: "getcurrentvideo.php", 
    success: function(data) 
	{
		document.getElementById('CurrentlyPlayingID').innerHTML = data;
		if (data == 'Nothing.')
		{
			document.getElementById('IDPlaybackControls').style.display = 'none';
		}
		else
		{
			document.getElementById('IDPlaybackControls').style.display = 'block';
		}	
    },
    complete: function() {
      // schedule the next request *only* when the current one is complete:
      setTimeout(myPeriodicMethod, 1000);
    }
  });
}

// schedule the first invocation:
setTimeout(myPeriodicMethod, 1000);