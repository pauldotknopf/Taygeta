using System;
using System.Collections.Concurrent;
using System.Threading.Tasks;
using Eventing.IPC;
using Microsoft.Practices.ServiceLocation;
using System.ComponentModel.Composition;
using Declarations;

namespace Taygeta.Processing
{
    /// <summary>
    /// Base class for custom processing nodes
    /// </summary>
    [InheritedExport]
    public abstract class ProcessingNode
    {
        protected ConcurrentBag<ProcessingNode> m_successors = new ConcurrentBag<ProcessingNode>();

        /// <summary>
        /// Determines whether this node is the first node that accepts video frames
        /// </summary>
        public bool IsRoot { get; set; }

        /// <summary>
        /// Determines whether this node has no successors
        /// </summary>
        public bool IsLeaf 
        { 
            get
            {
                return m_successors.Count == 0;
            }
        }
        
        protected ProcessingNode()
        {

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="item"></param>
        internal virtual void Handle(IBaseItem item)
        {
            IBaseItem result = item;
            Process(result);
            if (IsLeaf || !result.ShouldContinueProcessing)
            {
                result.Dispose();
                return;
            }

            if (result.ShouldContinueProcessing)
            {
                ContinueHandling(result);
            }
        }

        protected virtual void ContinueHandling(IBaseItem item)
        {
            foreach (var node in m_successors)
            {
                node.Handle(item);
            }
        }

        /// <summary>
        /// Adds immediate successors for processing output frames
        /// </summary>
        /// <param name="nodes"></param>
        public void AddSuccessors(params ProcessingNode[] nodes)
        {
            Array.ForEach(nodes, n => m_successors.Add(n));
        }

        /// <summary>
        /// Process next video frame
        /// </summary>
        /// <returns></returns>
        protected abstract void Process(IBaseItem input);

        /// <summary>
        /// Initializes node with service locator and event distributor instances
        /// </summary>
        /// <param name="serviceLocator"></param>
        /// <param name="eventRouter"></param>
        public abstract void Initialize(IServiceLocator serviceLocator, IEventRouter eventRouter);

        /// <summary>
        /// Invoked before video stream starts in order to set the frame format
        /// </summary>
        /// <param name="format"></param>
        public abstract void FormatSetup(BitmapFormat format);

        /// <summary>
        /// Gets the unique name of the node
        /// </summary>
        public abstract string Name { get; }
    }
}
